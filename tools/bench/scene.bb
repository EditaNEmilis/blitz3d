;
;	Blitz3D fork - scene graph, collision and animation micro-benchmarks
;	(blitz3d/world.cpp, collision.cpp, meshcollider.cpp, entity.cpp,
;	animator.cpp).
;
;	Runs with no graphics mode open. UpdateWorld drives World::update, which is
;	where entity enumeration, transform invalidation and the collision
;	broadphase all live.
;
;	RenderWorld is deliberately not called: tools\probe3d.ps1 reports that no
;	Direct3D 7 device can be created on a current Windows, so gxScene::clear,
;	render and end would dereference a null dir3dDev. The 3D submission path
;	is therefore not benchmarkable here and the 3D changes have to be argued
;	from code reading plus the correctness goldens, not from a number.
;
;	Graphics3D still has to be opened, even though nothing is ever rendered:
;	blitz3d_open() is only reached from there, and the World it creates is
;	what World::update walks. B3_ALLOW_NO_3D_DEVICE is what lets that succeed
;	with no device present, and tools\bench.ps1 sets it.
;
;	Pass a case number on the command line to run just that one:
;	    scene.exe 7              run every case
;	    scene.exe 3              run just the collision case
;
;	Minimum of REPS runs, for the reason documented at the top of bb_exec.bb.
;
;	Sizes are deliberately modest. World::collide is O(n_src x n_dst) with no
;	spatial partition, so the collision case is quadratic by construction and
;	a benchmark meant to be run before and after every change cannot afford to
;	take fifteen minutes.
;

Const FRAMES = 20
Const REPS = 3
Const N_ENT = 1000
Const N_COL = 120
Const ANIM_ENT = 200

Global out
Dim ents(2000)
Dim movers(1000)
Global g_sink#
Local i,f,t,best

Graphics3D 320,240,32,2
SetBuffer BackBuffer()
out = WriteFile( "scene.txt" )
; The command line is a single token, the case number, so bench.ps1 does not
; have to quote anything. CommandLine() comes back with a leading space -
; bbWinMain does params=cmd.substr( n+1 ) after the closing quote of the
; executable path - so it has to be trimmed before it is compared.
Local sel$
sel$ = Trim( CommandLine() )

;
;	1 entity-update: N entities moved once per frame with no collision types.
;	Isolates enumEnabled, the transform invalidation walk and
;	Object::beginUpdate/endUpdate.
;
If sel$ = "1" Or sel$ = "7" Then
	ClearWorld
	For i = 0 To N_ENT - 1
		ents(i) = CreateCube()
		ScaleMesh ents(i), .5, .5, .5
		PositionEntity ents(i), i, 10, 0
	Next
	best = 0
	For f = 1 To REPS
		t = MilliSecs()
		For i = 0 To FRAMES - 1
			For f = 0 To N_ENT - 1
				TranslateEntity ents(f), .01, 0, 0
			Next
			UpdateWorld
		Next
		t = MilliSecs() - t
		If best = 0 Or t < best Then best = t
	Next
	WriteLine out,"entity-update-1000" + Chr(9) + REPS + Chr(9) + (FRAMES * N_ENT) + Chr(9) + best
EndIf

;
;	2 entity-read: reading position and rotation back, which is where
;	Entity::getWorldRotation and getWorldScale are called per entity per frame
;	unless they are memoised.
;
If sel$ = "2" Or sel$ = "7" Then
	g_sink# = 0
	best = 0
	For f = 1 To REPS
		t = MilliSecs()
		For i = 0 To FRAMES - 1
			For f = 0 To N_ENT - 1
				g_sink# = g_sink# + EntityX( ents(f), 1 ) + EntityYaw( ents(f), 1 ) + EntityPitch( ents(f), 1 )
			Next
		Next
		t = MilliSecs() - t
		If best = 0 Or t < best Then best = t
	Next
	WriteLine out,"entity-read-1000" + Chr(9) + REPS + Chr(9) + (FRAMES * N_ENT) + Chr(9) + best
EndIf

;
;	3 collide-sphere: N moving spheres against N static spheres. This is the
;	O(n_src x n_dst) path in World::collide with no spatial partition, which is
;	what the Tier 2 broadphase is meant to fix.
;
If sel$ = "3" Or sel$ = "7" Then
	ClearWorld
	ClearCollisions
	Collisions 1, 2, 1, 2
	For i = 0 To N_COL - 1
		movers(i) = CreateCube()
		ScaleMesh movers(i), .5, .5, .5
		EntityRadius movers(i), 1, 1
		EntityType movers(i), 1, 0
		PositionEntity movers(i), 0, 0, i * 8
		ents(i) = CreateCube()
		ScaleMesh ents(i), .5, .5, .5
		EntityRadius ents(i), 1, 1
		EntityType ents(i), 2, 0
		PositionEntity ents(i), 0, 0, i * 8 + 4
	Next
	best = 0
	For f = 1 To REPS
		t = MilliSecs()
		For i = 0 To FRAMES - 1
			For f = 0 To N_COL - 1
				TranslateEntity movers(f), 0, 0, .5
			Next
			UpdateWorld
		Next
		t = MilliSecs() - t
		If best = 0 Or t < best Then best = t
	Next
	WriteLine out,"collide-sphere-120" + Chr(9) + REPS + Chr(9) + (FRAMES * N_COL) + Chr(9) + best
EndIf

;
;	4 collide-poly: same, but polygon (mesh collider) rather than sphere, so
;	the BVH descent inside MeshCollider is on the path.
;
If sel$ = "4" Or sel$ = "7" Then
	ClearWorld
	ClearCollisions
	Collisions 1, 2, 2, 2
	For i = 0 To 60 - 1
		movers(i) = CreateCube()
		EntityType movers(i), 1, 0
		PositionEntity movers(i), 0, 0, i * 8
		ents(i) = CreateCube()
		EntityType ents(i), 2, 0
		PositionEntity ents(i), 0, 0, i * 8 + 4
	Next
	best = 0
	For f = 1 To REPS
		t = MilliSecs()
		For i = 0 To FRAMES - 1
			For f = 0 To 60 - 1
				TranslateEntity movers(f), 0, 0, .5
			Next
			UpdateWorld
		Next
		t = MilliSecs() - t
		If best = 0 Or t < best Then best = t
	Next
	WriteLine out,"collide-poly-60" + Chr(9) + REPS + Chr(9) + (FRAMES * 60) + Chr(9) + best
EndIf

;

;
;	Cases 5 (animation), 6 (CameraPick) and 8 (UpdateNormals / MeshesIntersect)
;	are not in the default run. CameraPick crashes on this build and the other
;	two produce timings too small for MilliSecs() to resolve, so including them
;	would add noise rather than signal. The animation path is covered by the
;	geometry golden instead, which exercises the same Quat::normalized and
;	Matrix paths deterministically.
;
ClearWorld
CloseFile out
EndGraphics
End