;
;	Golden capture: entity/transform math.
;
;	This is the gate for the riskiest change in the whole plan - replacing the
;	shared 64-slot scratch rings that Matrix and Transform currently return
;	into (blitz3d/geom.h:257 and :399) with plain by-value returns. That change
;	is invisible from the outside, which is exactly what makes it dangerous:
;	nothing crashes, a wrong answer just propagates. A byte-identical dump is
;	the only way to know it landed correctly.
;
;	Everything here goes through Transform, Matrix, Quat and Vector: entity
;	transforms, the matrix element accessor, TFormPoint/TFormVector/
;	TFormNormal, DeltaPitch/DeltaYaw, EntityDistance, VectorPitch/VectorYaw,
;	mesh fitting and scaling. Chained expressions deliberately nest several
;	operator results, because the scratch ring is overwritten after 64
;	allocations and a chain is exactly what would expose it.
;
;	Output: one line per case, full float precision.

Global out
Global a, b, c, mesh
Local i, x#, y#, z#

out = WriteFile( "geometry.txt" )

;
;	Transform composition through entity hierarchies. Each level multiplies
;	the parent's matrix by its own, so a depth-4 chain is four Transform*
;	results alive at once.
;
a = CreateCube()
b = CreateCube()
c = CreateCube()
EntityParent b, a, 1
EntityParent c, b, 1
For i = 0 To 60
	RotateEntity a, i * 3, i * 5, i * 7, 1
	RotateEntity b, i * 2, i * 11, i * 13, 1
	ScaleEntity a, 1 + i * .01, 1 + i * .02, 1 + i * .03, 1
	PositionEntity b, i * .5, -i * .25, i * .75, 1
	PositionEntity c, i * 1.5, i * .25, -i * .5, 1
	WriteLine out,"xform " + i + " " + EntityX( c, 1 ) + " " + EntityY( c, 1 ) + " " + EntityZ( c, 1 )
	WriteLine out,"loc  " + i + " " + EntityX( c, 0 ) + " " + EntityY( c, 0 ) + " " + EntityZ( c, 0 )
	WriteLine out,"rot  " + i + " " + EntityPitch( c, 1 ) + " " + EntityYaw( c, 1 ) + " " + EntityRoll( c, 1 )
	WriteLine out,"scl  " + i + " " + GetMatElement( c, 1, 1 ) + " " + GetMatElement( c, 2, 2 ) + " " + GetMatElement( c, 3, 3 )
	WriteLine out,"off  " + i + " " + GetMatElement( c, 4, 1 ) + " " + GetMatElement( c, 4, 2 ) + " " + GetMatElement( c, 4, 3 )
	WriteLine out,"vec  " + i + " " + GetMatElement( c, 1, 2 ) + " " + GetMatElement( c, 2, 3 ) + " " + GetMatElement( c, 3, 1 )
Next

;
;	TFormPoint applies the inverse of one transform and the other. That is
;	Transform::operator~ (transpose) and Transform::operator- (inverse), both
;	of which currently write into the scratch ring.
;
For i = 1 To 40
	x# = i * 1.25
	y# = -i * .5
	z# = i * .0625
	TFormPoint x#, y#, z#, c, a
	WriteLine out,"tformp " + i + " " + TFormedX + " " + TFormedY + " " + TFormedZ
	TFormVector x#, y#, z#, c, a
	WriteLine out,"tformv " + i + " " + TFormedX + " " + TFormedY + " " + TFormedZ
	TFormNormal x#, y#, z#, c, a
	WriteLine out,"tformn " + i + " " + TFormedX + " " + TFormedY + " " + TFormedZ
Next

;
;	EntityDistance and the delta-angle helpers, which normalise quaternions
;	and therefore touch Quat::normalized - three divides today.
;
For i = 1 To 30
	PositionEntity a, Sqr( i ) * 3, i * -1.5, Sqr( i ) * .5, 1
	RotateEntity a, i * 4, i * 9, i * 2, 1
	RotateEntity b, -i * 3, i * 7, i * 5, 1
	WriteLine out,"dist " + i + " " + EntityDistance( a, b )
	WriteLine out,"dpitch " + i + " " + DeltaPitch( a, b )
	WriteLine out,"dyaw " + i + " " + DeltaYaw( a, b )
Next

;
;	VectorPitch and VectorYaw run a vector through the rotation extraction
;	path, which is Matrix transpose plus cofactor.
;
For i = 1 To 40
	x# = i * .125 - 2
	y# = i * -.25 + 1
	z# = i * .5 - 3
	WriteLine out,"vpitch " + i + " " + VectorPitch( x#, y#, z# )
	WriteLine out,"vyaw " + i + " " + VectorYaw( x#, y#, z# )
Next

;
;	PointEntity builds a full transform from a yaw/pitch/roll, and
;	AlignToVector does the same from a direction.
;
For i = 1 To 30
	PointEntity b, a, i * 6
	WriteLine out,"point " + i + " " + EntityPitch( b, 1 ) + " " + EntityYaw( b, 1 ) + " " + EntityRoll( b, 1 )
	AlignToVector c, i * 1.5, -i, i * .25, 1
	WriteLine out,"align " + i + " " + EntityPitch( c, 1 ) + " " + EntityYaw( c, 1 ) + " " + EntityRoll( c, 1 )
Next

;
;	Mesh building runs through Matrix too: FitMesh computes a bounding box
;	and ScaleMesh/RotateMesh/PositionMesh build transforms.
;
;	Note on arity: Blitz BASIC does not count a defaulted parameter towards
;	the accepted argument count for RotateMesh or ScaleMesh, so passing the
;	trailing global flag to those two is a compile error even though the
;	signature declares it. Four arguments is the accepted maximum.
;
;	UpdateNormals and the Vertex accessors are deliberately NOT in this
;	transcript. Surface::updateNormals accumulates through
;	std::map<Vector,Vector> keyed on an epsilon-based comparator that is not
;	a strict weak ordering, so it is undefined behaviour on meshes with
;	near-coincident vertices and is being rewritten anyway. Pinned
;	separately, and only after the rewrite.
;
mesh = CreateMesh()
For i = 0 To 30
	AddMesh mesh, CreateCube()
	RotateMesh mesh, i * 11, i * 13, i * 17
	ScaleMesh mesh, 1 + i * .1, 1 + i * .2, 1 + i * .3
	PositionMesh mesh, i * .3, i * -.4, i * .5
	WriteLine out,"mesh " + i + " " + MeshWidth( mesh ) + " " + MeshHeight( mesh ) + " " + MeshDepth( mesh )
Next
FitMesh mesh, 0, 0, 0, 10, 10, 10
WriteLine out,"fit  " + MeshWidth( mesh ) + " " + MeshHeight( mesh ) + " " + MeshDepth( mesh )

;
;	Long expression chains. These are what a 64-slot scratch ring would
;	overflow first, so they are deliberately deeper than any real program.
;
For i = 1 To 20
	RotateEntity a, i, i * 2, i * 3, 1
	RotateEntity b, i * 4, i * 5, i * 6, 1
	RotateEntity c, i * 7, i * 8, i * 9, 1
	x# = GetMatElement( a, 1, 1 ) * GetMatElement( b, 2, 2 ) + GetMatElement( c, 3, 3 )
	y# = GetMatElement( a, 1, 2 ) * GetMatElement( b, 2, 3 ) - GetMatElement( c, 3, 1 )
	z# = GetMatElement( a, 3, 1 ) * GetMatElement( b, 1, 3 ) + GetMatElement( c, 2, 2 )
	WriteLine out,"chain " + i + " " + x# + " " + y# + " " + z#
Next

ClearWorld
CloseFile out
End