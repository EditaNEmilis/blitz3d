;
;	Golden capture: collision.
;
;	The Tier 2 change that matters most here is a spatial broadphase for
;	World::collide, which is currently O(moving x all-collidable) with no
;	partition and rescans up to MAX_HITS=10 times per source object.
;
;	The danger is not that it gets slower or crashes. The danger is that a
;	reject which is one notch too tight silently changes which entity you
;	bounce off, and that shows up as "the game feels wrong" months later. So
;	every collision here is recorded with its normal and its contact time,
;	and the whole transcript has to stay byte-identical.
;
;	Two things about driving this from Blitz BASIC, both learned the hard way
;	and both load-bearing for anyone editing this file:
;
;	  * Collision indices are 1-based. bbCollisionX does
;	    getCollisions()[index-1], so index 0 reads out of bounds and faults.
;
;	  * Every scenario settles with one UpdateWorld before it moves anything.
;	    World::collide sweeps the segment from getPrevWorldTform() to
;	    getWorldTform(), and prev_tform is only set by Object::endUpdate. A
;	    scenario that teleports an entity and then calls UpdateWorld once has
;	    a prev_tform of wherever the entity was created, which is a segment
;	    the scenario never intended, and it silently produces no collisions.
;
;	Needs B3_ALLOW_NO_3D_DEVICE: there is no Direct3D 7 device on a current
;	Windows, but entities, transforms and collision are all engine-side.

Global out
Dim movers(100)
Dim statics(100)
Local i, k, n

Graphics3D 320,240,32,2
SetBuffer BackBuffer()
out = WriteFile( "collide.txt" )

;
;	1 Sphere against sphere, SLIDE response. The source is driven through the
;	target, so both the near and the far side get recorded.
;
ClearWorld
ClearCollisions
Collisions 1, 2, 1, 2
movers(0) = CreateCube()
EntityRadius movers(0), 1, 1
EntityType movers(0), 1, 0
statics(0) = CreateCube()
EntityRadius statics(0), 1, 1
EntityType statics(0), 2, 0
PositionEntity statics(0), 20, 0, 0
PositionEntity movers(0), -30, 0, 0
UpdateWorld 1.0
For i = 0 To 60
	PositionEntity movers(0), -30 + i, 0, 0, 1
	UpdateWorld 1.0
	WriteLine out,"s1 " + i + " cnt=" + CountCollisions( movers(0) ) + " x=" + EntityX( movers(0), 1 )
	For k = 1 To CountCollisions( movers(0) )
		WriteLine out,"s1c " + i + " " + k + " " + CollisionX( movers(0), k ) + " " + CollisionY( movers(0), k ) + " " + CollisionZ( movers(0), k ) + " " + CollisionNX( movers(0), k ) + " " + CollisionNY( movers(0), k ) + " " + CollisionNZ( movers(0), k ) + " " + CollisionTime( movers(0), k )
	Next
Next

;
;	2 Sphere against sphere, STOP response. The source ends up resting
;	against the target and never passes it, and it is the case where the
;	MAX_HITS loop in World::collide actually iterates more than once.
;
ClearWorld
ClearCollisions
Collisions 1, 2, 1, 1
movers(0) = CreateCube()
EntityRadius movers(0), 1, 1
EntityType movers(0), 1, 0
statics(0) = CreateCube()
EntityRadius statics(0), 1, 1
EntityType statics(0), 2, 0
PositionEntity statics(0), 10, 0, 0
PositionEntity movers(0), -30, 0, 0
UpdateWorld 1.0
For i = 0 To 40
	PositionEntity movers(0), -30 + i * 2, 0, 0, 1
	UpdateWorld 1.0
	WriteLine out,"s2 " + i + " " + EntityX( movers(0), 1 ) + " cnt=" + CountCollisions( movers(0) )
Next

;
;	3 Box against box, every response mode. Box collision is the expensive
;	narrowphase: Collision::boxCollide builds 24 normalised planes per call.
;
For n = 1 To 3
	ClearWorld
	ClearCollisions
	Collisions 1, 2, 3, n
	movers(0) = CreateCube()
	EntityBox movers(0), 0, 0, 0, 2, 2, 2
	EntityType movers(0), 1, 0
	statics(0) = CreateCube()
	EntityBox statics(0), 0, 0, 0, 4, 2, 4
	EntityType statics(0), 2, 0
	PositionEntity statics(0), 15, 0, 0
	PositionEntity movers(0), -30, 0, 0
	UpdateWorld 1.0
	For i = 0 To 40
		PositionEntity movers(0), -30 + i, i * .25, 0, 1
		UpdateWorld 1.0
		WriteLine out,"b3 " + n + " " + i + " " + EntityX( movers(0), 1 ) + " " + EntityY( movers(0), 1 ) + " " + EntityZ( movers(0), 1 ) + " cnt=" + CountCollisions( movers(0) )
	Next
Next

;
;	4 Ellipsoidal collision radii. These take the y_scale != 1 branch of
;	World::collide, where the destination transform is rebuilt on every retry
;	iteration as y_tform * dst_tform.
;
ClearWorld
ClearCollisions
Collisions 1, 2, 1, 2
For i = 0 To 24
	movers(i) = CreateCube()
	EntityRadius movers(i), 2, 1
	EntityType movers(i), 1, 0
	statics(i) = CreateCube()
	EntityRadius statics(i), 1, 4
	EntityType statics(i), 2, 0
	PositionEntity movers(i), 0, 0, -100 + i * 6
	PositionEntity statics(i), 0, 0, -97 + i * 6
Next
UpdateWorld 1.0
For i = 0 To 20
	For k = 0 To 24
		TranslateEntity movers(k), 0, 0, 1
	Next
	UpdateWorld 1.0
	For k = 0 To 24
		WriteLine out,"ell " + i + " " + k + " " + EntityZ( movers(k), 1 ) + " cnt=" + CountCollisions( movers(k) )
	Next
Next

;
;	5 Polygon collision, so the MeshCollider BVH descent is on the path.
;
ClearWorld
ClearCollisions
Collisions 1, 2, 2, 2
For i = 0 To 16
	movers(i) = CreateCube()
	EntityType movers(i), 1, 0
	statics(i) = CreateCube()
	EntityType statics(i), 2, 0
	PositionEntity movers(i), 0, 0, i * 10
	PositionEntity statics(i), 0, 0, i * 10 + 5
Next
UpdateWorld 1.0
For i = 0 To 20
	For k = 0 To 16
		TranslateEntity movers(k), 0, 0, 1
	Next
	UpdateWorld 1.0
	For k = 0 To 16
		WriteLine out,"poly " + i + " " + k + " " + EntityZ( movers(k), 1 ) + " cnt=" + CountCollisions( movers(k) )
	Next
Next

;
;	6 A long row of static targets, which is the case a broadphase has most to
;	reject and most opportunity to get wrong. Every step is recorded whether or
;	not it produced a hit.
;
ClearWorld
ClearCollisions
Collisions 1, 2, 1, 2
movers(0) = CreateCube()
EntityRadius movers(0), .5, .5
EntityType movers(0), 1, 0
For i = 0 To 40
	statics(i) = CreateCube()
	EntityRadius statics(i), .5, .5
	EntityType statics(i), 2, 0
	PositionEntity statics(i), i * 2, 0, 0
Next
PositionEntity movers(0), -10, 0, 0
UpdateWorld 1.0
For i = 0 To 60
	PositionEntity movers(0), -10 + i * 1.5, 0, 0, 1
	UpdateWorld 1.0
	WriteLine out,"row " + i + " " + CountCollisions( movers(0) ) + " " + EntityX( movers(0), 1 )
	For k = 1 To CountCollisions( movers(0) )
		WriteLine out,"rowc " + i + " " + k + " " + CollisionX( movers(0), k ) + " " + CollisionNX( movers(0), k )
	Next
Next

;
;	7 EntityCollided, which walks the reverse direction of the same table.
;
;	Its return value is deliberately reduced to a yes/no. bbEntityCollided
;	returns an Object* and Blitz BASIC carries that pointer as an integer
;	handle, so printing it puts a heap address in the transcript and ASLR
;	makes it differ on every run. That is the one line in this file that was
;	not reproducible until it was written this way.
;
ClearWorld
ClearCollisions
Collisions 1, 2, 1, 2
movers(0) = CreateCube()
EntityRadius movers(0), 1, 1
EntityType movers(0), 1, 0
statics(0) = CreateCube()
EntityRadius statics(0), 1, 1
EntityType statics(0), 2, 0
PositionEntity movers(0), -20, 0, 0
UpdateWorld 1.0
PositionEntity movers(0), 0, 0, 0, 1
UpdateWorld 1.0
WriteLine out,"ec1 " + CountCollisions( movers(0) ) + " " + CountCollisions( statics(0) )
If EntityCollided( movers(0), 2 ) = 0 Then WriteLine out,"ec1a 0"
If EntityCollided( movers(0), 2 ) <> 0 Then WriteLine out,"ec1a 1"
If EntityCollided( movers(0), 1 ) = 0 Then WriteLine out,"ec1b 0"
If EntityCollided( movers(0), 1 ) <> 0 Then WriteLine out,"ec1b 1"
If EntityCollided( statics(0), 1 ) = 0 Then WriteLine out,"ec1c 0"
If EntityCollided( statics(0), 1 ) <> 0 Then WriteLine out,"ec1c 1"
For k = 1 To CountCollisions( movers(0) )
	WriteLine out,"ec3 " + k + " " + CollisionTime( movers(0), k ) + " " + CollisionX( movers(0), k ) + " " + CollisionY( movers(0), k ) + " " + CollisionZ( movers(0), k ) + " " + CollisionNX( movers(0), k )
Next

;

;
;	Sections 8 (a grid of movers) and 9 (CameraPick) are not in this
;	transcript. The grid case faults partway through on this build, and
;	CameraPick faults on entry, both inside code this plan is about to change.
;	They are the obvious next things to pin once they stop faulting; until
;	then a transcript that stops halfway is worse than a shorter one that
;	completes.
;
ClearWorld
CloseFile out
EndGraphics
End