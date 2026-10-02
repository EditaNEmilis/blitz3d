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
;	Covered, and verified to be covered: Plot, Line, Rect, Oval, Text, DrawImage,
;	TileImage, TileBlock and CopyRect, as raw framebuffer bytes. If a rewrite
;	changes a single pixel of any of those, the hash moves.
;
;	NOT covered: the collision bitmask. Sections 7 and 8 below call
;	ImagesCollide and ImageRectCollide across the 32-pixel word boundary, but
;	that does not in fact reach gxCanvas::updateBitMask. This was established by
;	canary rather than assumed: forcing the 32bpp branch on and filling every
;	mask word with 0xDEADBEEF changed no collide answer in 2d.txt at all. The
;	most likely reason is the AABB rejection at gxcanvas.cpp:767 - colliding two
;	images at offsets 28..36 mostly rejects before the mask is ever read - but
;	the point is that it is currently unproven, and until it is proven these
;	lines are decoration, not a gate.
;
;	That matters because updateBitMask is exactly where a plausible-looking
;	rewrite goes quietly wrong: its inner loop folds each pixel into a word by
;	shifting left, so the pixel folded in first ends up in bit 31 and the last
;	in bit 0. A version that walks x downwards mirrors every word, and collide
;	then answers for the wrong pixels - no crash, no wrong count, just a
;	collision that disagrees with the image. The loop in the source now runs
;	ascending, and tools\verify.ps1 cannot currently tell you if that regresses.
;	Closing that gap is the next job on this file.
;
;	Everything drawn here is deliberately asymmetric. A symmetric pattern - a
;	centred dot, a square, an even-sized oval - is invariant under the word
;	mirroring that broke the mask, so it would have passed. Off-centre dots at
;	offsets that straddle the 32-pixel word boundary is what catches it.

Global out
Global g_a
Global g_b
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
;	7 The collision bitmask. Two identical 32x32 images with a single
;	asymmetric dot, collided at every offset across the word boundary. This is
;	the case that catches a mirrored mask word, and it is why the dots sit at
;	x=1 and x=30 rather than in the middle.
;
Cls
g_a = CreateImage( 32, 32 )
g_b = CreateImage( 32, 32 )
For j = 0 To 31
	WritePixel j, 0, 16711680, ImageBuffer( g_a )
	WritePixel j, 0, 16711680, ImageBuffer( g_b )
Next
For j = 0 To 31
	WritePixel 0, j, 65280, ImageBuffer( g_a )
	WritePixel 0, j, 65280, ImageBuffer( g_b )
Next

; The runtime calls these ImagesCollide and ImageRectCollide, not CollideImage
; and RectCollide. Both are 7 and 8 arguments; passing fewer is a compile error
; because Blitz BASIC does not count a defaulted parameter towards the arity.
;
WriteLine out,"collide baseline " + ImagesCollide( g_a, 0, 0, 0, g_b, 0, 0, 0 )

; The same dot, walked right one pixel at a time across and over the 32-pixel
; word boundary at x=32.
For i = 28 To 36
	WriteLine out,"collidex " + i + " " + ImagesCollide( g_a, i, 0, 0, g_b, 0, 0, 0 )
Next
For i = 28 To 36
	WriteLine out,"collidey " + i + " " + ImagesCollide( g_a, 0, i, 0, g_b, 0, 0, 0 )
Next

; Same again after the destination has been drawn into, which is what forces
; the lazy mask to be built from a dirty rectangle rather than up front.
Cls
Rect 0, 0, 320, 240, 1
For i = 28 To 36
	WriteLine out,"collidedirty " + i + " " + ImagesCollide( g_a, i, 0, 0, g_b, 0, 0, 0 )
Next
Plot 3, 3
Plot 35, 3
For i = 28 To 36
	WriteLine out,"collideafter " + i + " " + ImagesCollide( g_a, i, 0, 0, g_b, 0, 0, 0 )
Next

;
;	8 ImageRectCollide, which walks the same mask word by word in 32-pixel
;	chunks and is a different loop from ImagesCollide.
;
For i = 0 To 8
	WriteLine out,"rectcollide " + i + " " + ImageRectCollide( g_a, i * 4, 0, 0, 0, 0, 32, 32 )
Next

CloseFile out
EndGraphics
End