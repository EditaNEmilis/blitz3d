;
;	Golden capture: 2D canvas pixel output and the collision bitmask.
;
;	Why this exists when golden_core, golden_fmt and golden_collide already
;	exist: those pin the engine, the string library, float formatting and 3D
;	collision, and none of them touch a single pixel of 2D output. The 2D
;	rewrite - the lazy collision mask, the special-cased 32bpp mask loop, the
;	Oval scanline fill, the gxFont render signature - all landed with no gate
;	that can see whether they changed what is drawn.
;
;	That is not hypothetical. A plausible-looking rewrite of updateBitMask's
;	32bpp inner loop walked x downwards instead of up, which silently mirrors
;	each 32-pixel word. CollideImage then answers for the wrong pixels: no
;	crash, no wrong count, just a collision that disagrees with the image. The
;	smoke test passes, the benchmarks look fine, and every game with a sprite
;	vs sprite test is quietly wrong.
;
;	So: draw into the back buffer, SaveBuffer it, and let tools\verify.ps1 hash
;	the .bmp files. Any pixel difference anywhere changes the hash. This is
;	exact and it costs one file read.
;
;	WHAT THIS DOES AND DOES NOT COVER - read this before trusting it.
;
;	Covered, and verified to be covered: Plot, Line, Rect, Oval, Text,
;	DrawImage, TileImage, TileBlock and CopyRect, as raw framebuffer
;	bytes. If a rewrite changes a single pixel of any of those, the hash
;	moves.
;
;	Also covered, and proven covered by canary: the collision bitmask -
;	gxCanvas::updateBitMask and both loops that read it back,
;	collide() and rect_collide(). The first version of this file
;	called ImagesCollide across the 32-pixel word boundary with two
;	images that each had a solid top row, and a canary - filling every
;	mask word with 0xDEADBEEF - changed no answer in 2d.txt at all:
;	the AABB reject at the top of gxCanvas::collide answered before
;	the mask was ever read, and where the masks were read the solid
;	row made the answer "true" whatever the bits said. Those lines
;	were decoration, not a gate.
;
;	Sections 7 and 8 below are the replacement. Every image is cleared
;	to its mask colour by CreateImage, so it starts with no solid
;	pixels, and each test image carries one dot or a known
;	hand-computable set of dots. Two such images collide only when a
;	dot of one lands exactly on a dot of the other, so the transcript
;	depends on single bits of single 32-pixel words. A mask word
;	folded in the wrong order mirrors the dot to another column and
;	the "1" in the transcript moves; a missed rebuild of a dirty
;	region makes a dot that was just drawn invisible. Both are
;	caught, and the canary was re-run against this version to prove
;	it: the 0xDEADBEEF fill now changes the transcript.
;
;	Everything drawn here is deliberately asymmetric. A symmetric
;	pattern - a centred dot, a square, an even-sized oval - is
;	invariant under the word mirroring that broke the mask, so it
;	would have passed. Off-centre dots at offsets that straddle the
;	32-pixel word boundary is what catches it.

Global out
Global g_a
Global g_b
Global g_c
Global g_d
Global g_e
Global g_f
Global g_g
Global g_h
Local i, j, k

Graphics 320,240,32,2
SetBuffer BackBuffer()
out = WriteFile( "2d.txt" )

;
;	1 Plot: a single pixel, an ascending run, and a sparse diagonal. One-pixel
;	steps so an off-by-one in the origin or clip handling shows up.
;
Cls
For i = 0 To 319
	Plot i, (i * 7) Mod 239
Next
For i = 0 To 40
	Plot i, 3
	Plot i, 4
Next
Plot 0, 0
Plot 319, 239
Plot 0, 239
Plot 319, 0
SaveBuffer BackBuffer(), "2d_plot.bmp"

;
;	2 Line: all eight octants plus horizontal, vertical and the degenerate
;	zero-length line, because each takes a different branch of the Bresenham
;	split and a different clip path.
;
Cls
Line 0, 0, 319, 239
Line 0, 0, 319, 0
Line 0, 0, 0, 239
Line 0, 0, 319, 1
Line 0, 0, 1, 239
Line 0, 239, 319, 0
Line 319, 0, 0, 239
Line 5, 5, 5, 5
Line 5, 5, 5, 6
Line 100, 100, 100, 100
SaveBuffer BackBuffer(), "2d_line.bmp"

;
;	3 Rect: outline and solid, fully on-screen, and partly off-screen on each
;	of the four edges in turn, because the four edges are clipped individually
;	against the viewport and not against the rectangle as a whole.
;
Cls
Rect 10, 10, 60, 40, 0
Rect 100, 10, 60, 40, 1
Rect -20, 10, 60, 40, 0
Rect 280, 10, 60, 40, 0
Rect 10, -20, 60, 40, 0
Rect 10, 220, 60, 40, 0
Rect 0, 0, 1, 1, 0
Rect 318, 238, 2, 2, 1
SaveBuffer BackBuffer(), "2d_rect.bmp"

;
;	4 Oval: outline and solid at several sizes, all off-centre and all with
;	odd dimensions. The scanline fill computes a half-width per row from a
;	square root, so an even-sized oval cannot distinguish a sign error or a
;	half-pixel offset from the original.
;
Cls
Oval 7, 11, 53, 37, 0
Oval 90, 13, 71, 45, 1
Oval 180, 17, 33, 61, 0
Oval 13, 90, 97, 29, 1
Oval 140, 95, 41, 79, 0
Oval 3, 3, 3, 3, 0
Oval 3, 20, 3, 3, 1
SaveBuffer BackBuffer(), "2d_oval.bmp"

;
;	5 Text: a bitmap font, so this pins gxFont::render and gxCanvas::text,
;	including the clip-at-both-ends loops and the per-character colorkey blit.
;
Cls
Print "The quick brown fox jumps over the lazy dog"
Print "0123456789 !@#$%^&*()_+-=[]{};':,./<>?"
; The double quote goes in via Chr rather than an \" escape, so this file has no
; dependency on how the lexer handles one.
Print "quote=" + Chr( 34 ) + " backslash=" + Chr( 92 ) + " tab=" + Chr( 9 )
Print "jumps over the lazy dog"
SaveBuffer BackBuffer(), "2d_text.bmp"

;
;	6 Blits: a masked image drawn at an offset, tiled, and CopyRect'd. The mask
;	colour has to be honoured exactly or the dots bleed into the background.
;
Cls
g_a = CreateImage( 17, 11 )
MaskImage g_a, 255, 0, 255
WritePixel 1, 1, 16711680, ImageBuffer( g_a )
WritePixel 2, 3, 65280, ImageBuffer( g_a )
WritePixel 15, 9, 255, ImageBuffer( g_a )

DrawImage g_a, 3, 4
DrawImage g_a, 21, 26
DrawImage g_a, 200, 190
TileImage g_a, 2, 40
TileBlock g_a, 2, 60
CopyRect 40, 70, 60, 80, 20, 12, BackBuffer(), BackBuffer()
CopyRect 100, 120, 0, 0, 17, 11, BackBuffer(), ImageBuffer( g_a )
SaveBuffer BackBuffer(), "2d_blit.bmp"

;
;
;	7 The collision bitmask, read by gxCanvas::collide().
;
;	Every image here is 96x16 and cleared to black, which is also
;	its mask colour, so it starts with no solid pixels at all. A
;	written dot is the only solid pixel in the image, and two such
;	images collide only when a dot of one lands exactly on a dot of
;	the other. The answer therefore depends on single bits of single
;	32-pixel words, and a word folded in the wrong order mirrors the
;	dot to another column and moves the "1" in this transcript.
;
Cls
g_a = CreateImage( 96, 16 )
g_b = CreateImage( 96, 16 )
MaskImage g_a, 0, 0, 0
MaskImage g_b, 0, 0, 0

; One dot each: column 1 (word 0, bit 1) and column 48 (word 1,
; bit 16). The boxes overlap for every offset in -95..95 and the
; dots coincide at exactly one of them.
WritePixel 1, 0, 16711680, ImageBuffer( g_a )
WritePixel 48, 0, 16711680, ImageBuffer( g_b )

WriteLine out,"collide baseline " + ImagesCollide( g_a, 0, 0, 0, g_b, 0, 0, 0 )

; The whole overlap span, negative offsets included: x < 0 takes the
; branch of collide() that swaps the two canvases, x > 0 the branch
; that does not. The answer is 1 at exactly x=47.
For i = -50 To 50
	WriteLine out,"collidex " + i + " " + ImagesCollide( g_a, i, 0, 0, g_b, 0, 0, 0 )
Next

; Dots on the word boundaries: 31 is the last bit of word 0, 32 the
; first bit of word 1, 63 the last of word 1, 64 the first of word
; 2. The answer is 1 at exactly x=31, 32 and 33.
g_c = CreateImage( 96, 16 )
g_d = CreateImage( 96, 16 )
MaskImage g_c, 0, 0, 0
MaskImage g_d, 0, 0, 0
WritePixel 31, 0, 16711680, ImageBuffer( g_c )
WritePixel 32, 0, 16711680, ImageBuffer( g_c )
WritePixel 63, 0, 16711680, ImageBuffer( g_d )
WritePixel 64, 0, 16711680, ImageBuffer( g_d )
For i = 28 To 36
	WriteLine out,"collideword " + i + " " + ImagesCollide( g_c, i, 0, 0, g_d, 0, 0, 0 )
Next

; One dot in every word of one image, against a single dot: the
; answer is 1 at exactly x = 47, 17, 16, 15, -15, -16, -17, -46.
g_g = CreateImage( 96, 16 )
MaskImage g_g, 0, 0, 0
WritePixel 1, 0, 16711680, ImageBuffer( g_g )
WritePixel 31, 0, 16711680, ImageBuffer( g_g )
WritePixel 32, 0, 16711680, ImageBuffer( g_g )
WritePixel 33, 0, 16711680, ImageBuffer( g_g )
WritePixel 63, 0, 16711680, ImageBuffer( g_g )
WritePixel 64, 0, 16711680, ImageBuffer( g_g )
WritePixel 65, 0, 16711680, ImageBuffer( g_g )
WritePixel 94, 0, 16711680, ImageBuffer( g_g )
For i = -50 To 50
	WriteLine out,"collidemulti " + i + " " + ImagesCollide( g_g, i, 0, 0, g_b, 0, 0, 0 )
Next

; Vertically: dots at row 3 and row 11 of the same column, so the
; answer is 1 at exactly y=8 and mask rows 3 and 11 are the ones
; being read.
g_e = CreateImage( 96, 16 )
g_f = CreateImage( 96, 16 )
MaskImage g_e, 0, 0, 0
MaskImage g_f, 0, 0, 0
WritePixel 5, 3, 16711680, ImageBuffer( g_e )
WritePixel 5, 11, 16711680, ImageBuffer( g_f )
For i = 6 To 10
	WriteLine out,"collidey " + i + " " + ImagesCollide( g_e, 0, i, 0, g_f, 0, 0, 0 )
Next

; The lazy rebuild. The mask of g_a was built by the walks above.
; WritePixel flags the whole mask dirty (no rectangle to attribute
; the change to) and Plot, through damage(), unions a single 1x1
; rectangle; the next collide has to pick both new dots up.
; -29+77=48 and -42+90=48, so both collide lines below are 1, and a missed
; rebuild makes either of them 0.
WritePixel 77, 0, 16711680, ImageBuffer( g_a )
WriteLine out,"collidedirty " + ImagesCollide( g_a, -29, 0, 0, g_b, 0, 0, 0 )
SetBuffer ImageBuffer( g_a )
Plot 90, 0
SetBuffer BackBuffer()
WriteLine out,"collideafter " + ImagesCollide( g_a, -42, 0, 0, g_b, 0, 0, 0 )
WriteLine out,"collidestill " + ImagesCollide( g_a, 47, 0, 0, g_b, 0, 0, 0 )

;
;	8 ImageRectCollide, which walks the same mask words through
;	rect_collide()'s loop - FWMS/LWMS masked ends instead of the
;	shifted carry - and is a different code path from section 7.
;
;	g_h carries one dot at column 31, so a 1x16 rectangle hits it at
;	exactly rect_x=31, and g_g carries a dot in every word for the
;	wide-rectangle walks.
;
g_h = CreateImage( 96, 16 )
MaskImage g_h, 0, 0, 0
WritePixel 31, 0, 16711680, ImageBuffer( g_h )

; The rectangle walked across the dot: 1 at exactly rect_x=31.
For i = 28 To 34
	WriteLine out,"rectcollide " + i + " " + ImageRectCollide( g_h, 0, 0, 0, i, 0, 1, 16 )
Next

; The image walked across a fixed rectangle: 1 at exactly x=0.
For i = -2 To 2
	WriteLine out,"rectwalk " + i + " " + ImageRectCollide( g_h, i, 0, 0, 31, 0, 1, 16 )
Next

; Wide rectangles over every word of g_g, whose dot columns are 1,
; 31, 32, 33, 63, 64, 65 and 94. A 32-wide rectangle covers at
; least one dot everywhere except rect_x=95, where it covers just
; column 95, and rect_x=96, where the boxes no longer overlap.
WriteLine out,"rectwide " + ImageRectCollide( g_g, 0, 0, 0, 0, 0, 32, 16 )
WriteLine out,"rectwide " + ImageRectCollide( g_g, 0, 0, 0, 2, 0, 32, 16 )
WriteLine out,"rectwide " + ImageRectCollide( g_g, 0, 0, 0, 32, 0, 32, 16 )
WriteLine out,"rectwide " + ImageRectCollide( g_g, 0, 0, 0, 34, 0, 32, 16 )
WriteLine out,"rectwide " + ImageRectCollide( g_g, 0, 0, 0, 64, 0, 32, 16 )
WriteLine out,"rectwide " + ImageRectCollide( g_g, 0, 0, 0, 66, 0, 32, 16 )
WriteLine out,"rectwide " + ImageRectCollide( g_g, 0, 0, 0, 95, 0, 32, 16 )
WriteLine out,"rectwide " + ImageRectCollide( g_g, 0, 0, 0, 96, 0, 32, 16 )

; The same incremental rebuild as above, through the rect_collide
; path this time: the dot Plot just drew into g_a at (90,0).
WriteLine out,"rectdirty " + ImageRectCollide( g_a, 0, 0, 0, 90, 0, 1, 1 )

CloseFile out
EndGraphics
End
