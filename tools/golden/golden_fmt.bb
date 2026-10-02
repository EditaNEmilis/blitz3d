;
;	Golden capture: float formatting and the BASIC string library.
;
;	ftoa() in stdutil/stdutil.cpp is the most executed allocation site in the
;	runtime and is being rewritten to format into a char buffer instead of
;	building five to seven std::string temporaries. This file pins its exact
;	output, including the awkward cases, because "roughly the same number" is
;	not good enough when a game prints coordinates or scores.
;
;	bbstring.cpp's Left/Right/Mid/Trim/LSet/RSet/String$ are getting the same
;	treatment: substr temporaries removed, RSet's O(n^2) replaced with insert.
;	Every edge case those functions document is pinned here.
;
;	Output: ftoa.txt and strings.txt.

Global out
Global a$
Local i#, k, c

out = WriteFile( "ftoa.txt" )

;
;	Fixed values first, because these are the ones with formatting corner
;	cases: zero, negatives, the sign of zero, tiny and huge magnitudes,
;	repeating fractions, and the rounding boundaries.
;
WriteLine out,"zero  " + 0.0
WriteLine out,"nzero " + -0.0
WriteLine out,"one   " + 1.0
WriteLine out,"none  " + -1.0
WriteLine out,"half  " + 0.5
WriteLine out,"third " + 1.0/3.0
WriteLine out,"twoth " + 2.0/3.0
WriteLine out,"tenth " + 1.0/10.0
WriteLine out,"e5    " + 0.00001
WriteLine out,"e6    " + 0.000001
WriteLine out,"e7    " + 0.0000001
WriteLine out,"e8    " + 0.00000001
WriteLine out,"big7  " + 10000000.0
WriteLine out,"big8  " + 100000000.0
WriteLine out,"big9  " + 1000000000.0
;	Blitz BASIC has no exponent literal - 1.0e15 lexes as 1.0 followed by an
;	identifier - so the big magnitudes are spelled out.
WriteLine out,"huge  " + 1000000000000000.0
WriteLine out,"huge2 " + 10000000000000000.0
WriteLine out,"tiny  " + 0.000000000001
WriteLine out,"neg   " + -123456.789
WriteLine out,"pos   " + 123456.789
WriteLine out,"sqr2  " + Sqr( 2 )
WriteLine out,"sqr3  " + Sqr( 3 )
WriteLine out,"exp1  " + Exp( 1 )
WriteLine out,"exp10 " + Exp( 10 )
WriteLine out,"log2  " + Log( 2 )
WriteLine out,"log2b " + Log10( 2 )

;
;	A dense sweep. Every one of these goes through the same code path, and a
;	change in digit count or exponent handling shows up as a diff.
;
For i = -500 To 500
	WriteLine out,"sw1 " + i + " " + i * 0.1
Next
For i = -200 To 200
	WriteLine out,"sw2 " + i + " " + i * 0.000001
Next
For i = -100 To 100
	WriteLine out,"sw3 " + i + " " + i * 12345.6789
Next
For i = 1 To 100
	WriteLine out,"sw4 " + i + " " + Sqr( i ) * Exp( i * 0.01 )
Next

CloseFile out

out = WriteFile( "strings.txt" )

;
;	Left and Right, sweeping the count past the end of the string, where
;	Right's documented clamp leaves the value alone. Counts start at 1
;	because the CHKPOS guard rejects zero.
;
a$ = "hello world"
For k = 1 To 15
	WriteLine out,"left " + k + " [" + Left( a$, k ) + "]"
	WriteLine out,"right " + k + " [" + Right( a$, k ) + "]"
Next
a$ = ""
WriteLine out,"leftempty [" + Left( a$, 1 ) + "]"
a$ = ""
WriteLine out,"rightempty [" + Right( a$, 1 ) + "]"

;
;	Mid with an explicit count, and with the documented negative count that
;	means "to the end". The start index starts at 1 for the same CHKOFF
;	reason, but the count may be zero or negative.
;
a$ = "hello world"
For k = 1 To 15
	WriteLine out,"mid " + k + " [" + Mid( a$, k ) + "]"
	WriteLine out,"midc " + k + " [" + Mid( a$, k, 4 ) + "]"
	WriteLine out,"mid0 " + k + " [" + Mid( a$, k, 0 ) + "]"
	WriteLine out,"midn " + k + " [" + Mid( a$, k, -1 ) + "]"
	WriteLine out,"midfar " + k + " [" + Mid( a$, k, 99 ) + "]"
Next
a$ = ""
WriteLine out,"midempty [" + Mid( a$, 1 ) + "]"

;
;	Trim over every kind of whitespace and every kind of non-whitespace at
;	the edges, because Trim is about scanning, not copying.
;
a$ = "  hello  "
WriteLine out,"trim1 [" + Trim( a$ ) + "]"
a$ = "hello"
WriteLine out,"trim2 [" + Trim( a$ ) + "]"
a$ = "   "
WriteLine out,"trim3 [" + Trim( a$ ) + "]"
a$ = ""
WriteLine out,"trim4 [" + Trim( a$ ) + "]"
a$ = Chr(9) + "tab" + Chr(13) + Chr(10)
WriteLine out,"trim5 [" + Trim( a$ ) + "]"
a$ = " " + Chr(8) + Chr(11) + Chr(12) + " x " + Chr(8) + " "
WriteLine out,"trim6 [" + Trim( a$ ) + "]"

;
;	LSet and RSet. RSet used to copy the whole string once per padding
;	character, so the large cases matter.
;
;	Argument constraints, which are the library's own CHKPOS/CHKOFF guards and
;	which raise a runtime error rather than returning anything:
;	  Left/Right/LSet/RSet  require > 0
;	  Mid                   requires > 0
;	So the sweeps start at 1, not 0 and not -3. Anything below would raise
;	"parameter must be positive" and stop the program, which is correct
;	behaviour and not something to pin.
;
For k = 1 To 12
	a$ = "abc"
	WriteLine out,"lset " + k + " [" + LSet( a$, k ) + "]"
	a$ = "abc"
	WriteLine out,"rset " + k + " [" + RSet( a$, k ) + "]"
Next
a$ = "abc"
WriteLine out,"lsetbig [" + LSet( a$, 300 ) + "]"
a$ = "abc"
WriteLine out,"rsetbig [" + RSet( a$, 300 ) + "]"
a$ = ""
WriteLine out,"lsetempty [" + LSet( a$, 1 ) + "]"
a$ = ""
WriteLine out,"rsetempty [" + RSet( a$, 1 ) + "]"

;
;	String$ repeat, including the zero and negative cases.
;
WriteLine out,"str0  [" + String$( "xy", 0 ) + "]"
WriteLine out,"str1  [" + String$( "xy", 1 ) + "]"
WriteLine out,"str5  [" + String$( "xy", 5 ) + "]"
WriteLine out,"strm1 [" + String$( "xy", -1 ) + "]"
WriteLine out,"stre0 [" + String$( "", 3 ) + "]"

;
;	The rest of the library, unchanged but worth pinning.
;
a$ = "Hello, World"
WriteLine out,"upper [" + Upper( a$ ) + "]"
WriteLine out,"lower [" + Lower( a$ ) + "]"
WriteLine out,"hex0  [" + Hex( 0 ) + "]"
WriteLine out,"hex255 [" + Hex( 255 ) + "]"
WriteLine out,"hexneg [" + Hex( -1 ) + "]"
WriteLine out,"hexbig [" + Hex( 2147483647 ) + "]"
WriteLine out,"asc  " + Asc( "A" ) + " " + Asc( "" ) + " " + Asc( Chr( 255 ) )
WriteLine out,"len  " + Len( a$ )
WriteLine out,"chr  [" + Chr( 65 ) + "]"
WriteLine out,"instr " + Instr( a$, "World" ) + " " + Instr( a$, "zzz" )
WriteLine out,"replace [" + Replace( "a-b-c", "-", "+" ) + "]"
WriteLine out,"replace2 [" + Replace( "aaa", "a", "bb" ) + "]"
WriteLine out,"replace3 [" + Replace( "abc", "x", "y" ) + "]"

;
;	Concatenation, which _bbStrLoad and _bbStrConcat own. The character is
;	walked with a counter rather than Mod(), because Mod is a keyword and
;	Mod( a, b ) does not parse as a call.
;
a$ = ""
c = 0
For k = 1 To 40
	c = c + 1
	If c > 26 Then c = 1
	a$ = a$ + Chr( 64 + c )
	WriteLine out,"cat " + k + " [" + a$ + "] len=" + Len( a$ )
Next
;
;	Comparison has no expression form in Blitz BASIC, so it is pinned through
;	If statements instead: -1 is True, 0 is False.
;
a$ = "abc"
If a$ = "abc" Then WriteLine out,"cmp1 1" Else WriteLine out,"cmp1 0"
If a$ = "abd" Then WriteLine out,"cmp2 1" Else WriteLine out,"cmp2 0"
If "" < "a" Then WriteLine out,"cmp3 1" Else WriteLine out,"cmp3 0"
If "b" > "a" Then WriteLine out,"cmp4 1" Else WriteLine out,"cmp4 0"
If "a" < "b" Then WriteLine out,"cmp5 1" Else WriteLine out,"cmp5 0"
If "" < "" Then WriteLine out,"cmp6 1" Else WriteLine out,"cmp6 0"
If "abc" < "abcd" Then WriteLine out,"cmp7 1" Else WriteLine out,"cmp7 0"

CloseFile out
End