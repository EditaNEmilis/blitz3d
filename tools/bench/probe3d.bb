;
;	Does a Direct3D 7 scene exist on this machine?
;
;	gxGraphics::createScene() (gxruntime/gxgraphics.cpp:519) asks
;	DirectDraw 7 for IID_IDirect3D7 and enumerates devices. If it cannot,
;	it returns 0 and every 3D path in the engine dereferences a null
;	dir3dDev. gxScene::clear/render/end (gxscene.cpp:554,559,601) have no
;	null check at all.
;
;	So this program either writes the answer and exits 0, or it hangs on the
;	runtime's modal "Error!" box. tools\probe3d.ps1 tells the two apart by
;	timeout, which is the same trick tools\smoketest.ps1 uses for the
;	320x240 exclusive case.
;
;	Everything in the plan that measures 3D submission depends on the answer
;	to this file.

Global out
out = WriteFile( "probe3d.txt" )

Graphics3D 320,240,32,2
SetBuffer BackBuffer()

; If we got here a device exists. Prove it by asking for a camera, a light
; and a mesh entity and rendering a frame.
Global cam
cam = CreateCamera()
Global light
light = CreateLight()
Global mesh
mesh = CreateMesh()
AddMesh mesh, CreateCube()

PositionEntity cam, 0, 0, -10

For f = 1 To 5
	TurnEntity mesh, 5, 2, 0
	RenderWorld
	RenderScene
	Flip
Next

WriteLine out,"graphics3d: yes"
WriteLine out,"width=" + GraphicsWidth()
WriteLine out,"height=" + GraphicsHeight()
WriteLine out,"depth=" + GraphicsDepth()
WriteLine out,"tris=" + TrisDrawn()

CloseFile out
EndGraphics
End