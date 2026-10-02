;
;	Blitz3D fork - 2D canvas micro-benchmarks (gxruntime/gxcanvas.cpp).
;
;	Everything here goes through bbruntime/bbgraphics.cpp into gxCanvas. The
;	draws are timed without Flip, because Flip is a DirectDraw present and
;	would swamp the per-primitive cost being measured.
;
;	Minimum of REPS runs, for the reason documented at the top of bb_exec.bb:
;	small hot loops in Blitz3D's generated code are highly sensitive to where
;	the register allocator happens to place them.

Const N_PLOT = 20000
Const N_LINE = 20000
Const N_RECT = 4000
Const N_OVAL = 400
Const N_TEXT = 4000
Const N_PRINT = 4000
Const N_TILE = 200
Const N_COPYRECT = 4000
Const REPS = 5

Global out
Global g_img
Global g_hit
Local r,i,t,best

Graphics 800,600,32,2
SetBuffer BackBuffer()

; A small masked image so DrawImage/TileImage exercise the colorkey blit path
; rather than the solid one. Built in memory rather than loaded from a file so
; the benchmark has no external dependency and cannot fail on a missing path.
; If LoadImage ever does return 0 here, every image case below is drawing with
; a null image and the numbers are meaningless.
g_img = CreateImage( 32,32 )
MaskImage g_img, 0, 255, 0
WritePixel 1, 1, 16711680, ImageBuffer( g_img )
WritePixel 16, 16, 16711680, ImageBuffer( g_img )
Cls
out = WriteFile( "canvas2d.txt" )

Cls
best = 0
For r = 1 To REPS
	t = MilliSecs()
	For i = 1 To N_PLOT
		Plot i, i
	Next
	t = MilliSecs() - t
	If best = 0 Or t < best Then best = t
Next
WriteLine out,"plot" + Chr(9) + REPS + Chr(9) + N_PLOT + Chr(9) + best

best = 0
For r = 1 To REPS
	t = MilliSecs()
	For i = 1 To N_LINE
		Line 0, 0, 799, 599
	Next
	t = MilliSecs() - t
	If best = 0 Or t < best Then best = t
Next
WriteLine out,"line" + Chr(9) + REPS + Chr(9) + N_LINE + Chr(9) + best

best = 0
For r = 1 To REPS
	t = MilliSecs()
	For i = 1 To N_RECT
		Rect 10, 10, 60, 40, 0
	Next
	t = MilliSecs() - t
	If best = 0 Or t < best Then best = t
Next
WriteLine out,"rect-outline" + Chr(9) + REPS + Chr(9) + N_RECT + Chr(9) + best

best = 0
For r = 1 To REPS
	t = MilliSecs()
	For i = 1 To N_RECT
		Rect 10, 10, 60, 40, 1
	Next
	t = MilliSecs() - t
	If best = 0 Or t < best Then best = t
Next
WriteLine out,"rect-solid" + Chr(9) + REPS + Chr(9) + N_RECT + Chr(9) + best

best = 0
For r = 1 To REPS
	t = MilliSecs()
	For i = 1 To N_OVAL
		Oval 10, 10, 120, 90, 0
	Next
	t = MilliSecs() - t
	If best = 0 Or t < best Then best = t
Next
WriteLine out,"oval-outline" + Chr(9) + REPS + Chr(9) + N_OVAL + Chr(9) + best

best = 0
For r = 1 To REPS
	t = MilliSecs()
	For i = 1 To N_OVAL
		Oval 10, 10, 120, 90, 1
	Next
	t = MilliSecs() - t
	If best = 0 Or t < best Then best = t
Next
WriteLine out,"oval-solid" + Chr(9) + REPS + Chr(9) + N_OVAL + Chr(9) + best

best = 0
For r = 1 To REPS
	t = MilliSecs()
	For i = 1 To N_TEXT
		Text 10, 10, "benchmark string"
	Next
	t = MilliSecs() - t
	If best = 0 Or t < best Then best = t
Next
WriteLine out,"text" + Chr(9) + REPS + Chr(9) + N_TEXT + Chr(9) + best

; Print is the one every HUD and every debug print goes through, so it also
; covers startPrinting()/endPrinting() and the per-print surface lock.
ClsColor 0, 0, 0
best = 0
For r = 1 To REPS
	t = MilliSecs()
	For i = 1 To N_PRINT
		Print "score=" + i
	Next
	t = MilliSecs() - t
	If best = 0 Or t < best Then best = t
Next
WriteLine out,"print" + Chr(9) + REPS + Chr(9) + N_PRINT + Chr(9) + best

best = 0
For r = 1 To REPS
	t = MilliSecs()
	For i = 1 To N_TILE
		TileImage g_img, 0, 0
	Next
	t = MilliSecs() - t
	If best = 0 Or t < best Then best = t
Next
WriteLine out,"tileimage" + Chr(9) + REPS + Chr(9) + N_TILE + Chr(9) + best

best = 0
For r = 1 To REPS
	t = MilliSecs()
	For i = 1 To N_COPYRECT
		CopyRect 200, 200, 128, 128, 220, 220
	Next
	t = MilliSecs() - t
	If best = 0 Or t < best Then best = t
Next
WriteLine out,"copyrect" + Chr(9) + REPS + Chr(9) + N_COPYRECT + Chr(9) + best

; ImagesCollide allocates gxCanvas::cm_mask. Once it has been allocated,
; gxScene::end() -> gxCanvas::damage() rebuilds the whole bitmask every frame,
; so this is the case that measures that.
;
; It is deliberately image-versus-image and not image-versus-BackBuffer():
; the buffer form crashes on this build, inside gxCanvas::collide, which is a
; bug in its own right and not something a benchmark should be built on.
Cls
best = 0
For r = 1 To REPS
	t = MilliSecs()
	For i = 1 To N_PLOT
		Plot i, i
		g_hit = ImagesCollide( g_img, 0, 0, 0, g_img, i, i, 0 )
	Next
	t = MilliSecs() - t
	If best = 0 Or t < best Then best = t
Next
WriteLine out,"plot+imagecollide" + Chr(9) + REPS + Chr(9) + N_PLOT + Chr(9) + best

CloseFile out
EndGraphics
End