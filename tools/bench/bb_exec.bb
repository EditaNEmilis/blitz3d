;	Blitz3D fork - BASIC execution micro-benchmarks.
;
;	Isolates the cost of the generated code itself and of the runtime support
;	library (bbruntime/basic.cpp), with no graphics and no scene graph in the
;	way. Results go to the file named on the command line, because Print draws
;	to a window and scraping a window per run is not measurable.
;
;	REPETITIONS, and why this is not optional
;	--------------------------------------
;	Blitz3D's generated code is laid out by a tree walking register allocator
;	with no loop alignment, so a small hot loop's measured cost depends on
;	which address it happens to land at. Adding one unused function ahead of
;	the benchmark was measured to move a 1M-iteration 8-argument call loop
;	from 8ms to 156ms on the same machine, same runtime, same source. That is
;	a 20x swing from code placement alone.
;
;	So every case runs REPS times and reports the MINIMUM. The minimum is the
;	layout-independent, cache-warm, frequency-boosted best case, and it is
;	stable to a few percent. The median of a handful of runs is not.
;
;	Output format, one case per line:
;		name <TAB> reps <TAB> iterations <TAB> best_ms
;
;	tools\bench.ps1 turns that into operations per second.

Const N_LOOP = 5000000
Const N_CALL0 = 1000000
Const N_CALL8 = 1000000
Const N_CONCAT = 500000
Const N_FMTI = 500000
Const N_FMTF = 200000
Const REPS = 7

Global out
Global g_sink
Global g_out$
Global g_a$
Global g_b$

Local r,i,n,t,best

Function f0()
	g_sink = g_sink + 1
	Return g_sink
End Function

Function f8(a,b,c,d,e,f,g,h)
	Local s
	s = a + b + c + d + e + f + g + h
	g_sink = s
	Return s
End Function

;	The float variant is a separate case because Blitz3D passes every float
;	argument through the x87 stack: the generated code does
;	"push reg / fld [esp] / pop reg" per argument, which is why it costs an
;	order of magnitude more than the integer form.
Function f8f(a#,b#,c#,d#,e#,f#,g#,h#)
	Local s#
	s# = a# + b# + c# + d# + e# + f# + g# + h#
	g_sink = 1
	Return s#
End Function

g_a$ = "hello world, "
g_b$ = "and hello again"
out = WriteFile( CommandLine() )

;
;	loop-basic: empty loop body plus one global store. The floor for anything
;	that runs BASIC.
;
n = N_LOOP
best = 0
For r = 1 To REPS
	g_sink = 0
	t = MilliSecs()
	For i = 1 To n
		g_sink = g_sink + 1
	Next
	t = MilliSecs() - t
	If best = 0 Or t < best Then best = t
Next
WriteLine out,"loop-basic" + Chr(9) + REPS + Chr(9) + n + Chr(9) + best

;
;	call-0: cost of a Blitz function call with no arguments.
;
n = N_CALL0
best = 0
For r = 1 To REPS
	g_sink = 0
	t = MilliSecs()
	For i = 1 To n
		f0()
	Next
	t = MilliSecs() - t
	If best = 0 Or t < best Then best = t
Next
WriteLine out,"call-0" + Chr(9) + REPS + Chr(9) + n + Chr(9) + best

;
;	call-8: eight integer arguments.
;
n = N_CALL8
best = 0
For r = 1 To REPS
	g_sink = 0
	t = MilliSecs()
	For i = 1 To n
		f8(1,2,3,4,5,6,7,8)
	Next
	t = MilliSecs() - t
	If best = 0 Or t < best Then best = t
Next
WriteLine out,"call-8" + Chr(9) + REPS + Chr(9) + n + Chr(9) + best

;
;	call-8f: the same call with float arguments.
;
n = N_CALL8
best = 0
For r = 1 To REPS
	t = MilliSecs()
	For i = 1 To n
		f8f(1,2,3,4,5,6,7,8)
	Next
	t = MilliSecs() - t
	If best = 0 Or t < best Then best = t
Next
WriteLine out,"call-8f" + Chr(9) + REPS + Chr(9) + n + Chr(9) + best

;
;	string-concat: plain append of two long-ish globals. Exercises
;	_bbStrLoad twice and _bbStrConcat once, above the 15 char SSO threshold,
;	so the heap path is the one being measured.
;
n = N_CONCAT
best = 0
For r = 1 To REPS
	t = MilliSecs()
	For i = 1 To n
		g_out$ = g_a$ + g_b$
	Next
	t = MilliSecs() - t
	If best = 0 Or t < best Then best = t
Next
WriteLine out,"string-concat" + Chr(9) + REPS + Chr(9) + n + Chr(9) + best

;
;	string-fmt-int: int to string (bbHex) plus concatenation.
;
n = N_FMTI
best = 0
For r = 1 To REPS
	t = MilliSecs()
	For i = 1 To n
		g_out$ = "n=" + Hex(i)
	Next
	t = MilliSecs() - t
	If best = 0 Or t < best Then best = t
Next
WriteLine out,"string-fmt-int" + Chr(9) + REPS + Chr(9) + n + Chr(9) + best

;
;	string-fmt-float: the float path, which is stdutil ftoa() and is the most
;	executed allocation site in the runtime.
;
n = N_FMTF
best = 0
For r = 1 To REPS
	t = MilliSecs()
	For i = 1 To n
		g_out$ = "n=" + (i * 1.5)
	Next
	t = MilliSecs() - t
	If best = 0 Or t < best Then best = t
Next
WriteLine out,"string-fmt-float" + Chr(9) + REPS + Chr(9) + n + Chr(9) + best

;
;	string-funcs: Left/Right/Mid/Trim/LSet/RSet, the bbruntime/bbstring.cpp
;	predicates and the assign-in-place opportunities.
;
n = N_FMTI
best = 0
For r = 1 To REPS
	t = MilliSecs()
	For i = 1 To n
		g_out$ = Trim(RSet(g_a$,40))
	Next
	t = MilliSecs() - t
	If best = 0 Or t < best Then best = t
Next
WriteLine out,"string-funcs" + Chr(9) + REPS + Chr(9) + n + Chr(9) + best

CloseFile out
End