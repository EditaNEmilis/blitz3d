;
;	Golden capture: the .x model loader.
;
;	Why this exists: .x model files stopped loading when the DirectX SDK
;	and d3dxof.dll went away, and the loader that replaced them is a
;	from-scratch reader for a format nobody here had a test for. The
;	reader has two encodings (text and binary), several templates
;	(Mesh, MeshNormals, MeshTextureCoords, MeshMaterialList, Material,
;	TextureFilename, Frame, FrameTransformMatrix, AnimationSet), and
;	references between objects. A change that shifts a vertex, drops a
;	face, loses the texture coordinates or reorders the surfaces would
;	still leave a program that runs and exits cleanly, which is exactly
;	the failure mode the other goldens exist to catch.
;
;	Two models are covered.
;
;	The first is written by this program, so the text encoding is
;	pinned against a file whose every byte is known here: a cube with
;	normals and texture coordinates. It is written to the working
;	directory, so the case needs nothing on disk.
;
;	The second is a real model from the release tree, passed on the
;	command line, because it is the interesting one: a frame hierarchy
;	with a material list, a texture, and a mesh whose faces are not all
;	triangles. The path is not in the transcript, only the geometry, so
;	the recording does not depend on where the tree lives.
;
;	What is pinned: the vertex and triangle counts, every vertex of the
;	cube (position, texture coordinate and normal), and for the real
;	model the surface count, the per surface counts, the bounding box
;	and the first few vertices. The normals of the cube come from the
;	file and the normals of the real model from the file as well, so a
;	reader that stops applying them changes the transcript.
;
;	Loading a model needs no graphics device: the texture a material
;	names is loaded into a canvas, and a canvas needs a device, but the
;	device is only asked for its caps, which is the one place the
;	Direct3D 7 path used to crash when there was no device at all.
;	tools\verify.ps1 sets B3_ALLOW_NO_3D_DEVICE, and the transcript
;	does not depend on whether the texture could be loaded.

Global out
out = WriteFile( "xmodel.txt" )

;
;	1. the cube, written here and loaded back
;
Local cube = WriteFile( "cube.x" )
WriteLine cube, "xof 0302txt 0032"
WriteLine cube, "Mesh cube {"
WriteLine cube, " 8;"
WriteLine cube, " -1.0;1.0;1.0;, -1.0;-1.0;1.0;, 1.0;-1.0;1.0;, 1.0;1.0;1.0;,"
WriteLine cube, " -1.0;1.0;-1.0;, -1.0;-1.0;-1.0;, 1.0;-1.0;-1.0;, 1.0;1.0;-1.0;;"
WriteLine cube, " 6;"
WriteLine cube, " 3;0;1;2;, 3;0;2;3;, 3;4;6;5;, 3;4;7;6;, 3;0;4;7;, 3;0;7;3;;"
WriteLine cube, " MeshNormals {"
WriteLine cube, "  8;"
WriteLine cube, "  0.0;0.0;1.0;, 0.0;0.0;1.0;, 0.0;0.0;1.0;, 0.0;0.0;1.0;,"
WriteLine cube, "  0.0;0.0;-1.0;, 0.0;0.0;-1.0;, 0.0;0.0;-1.0;, 0.0;0.0;-1.0;;"
WriteLine cube, " }"
WriteLine cube, " MeshTextureCoords {"
WriteLine cube, "  8;"
WriteLine cube, "  0.0;0.0;, 0.0;1.0;, 1.0;1.0;, 1.0;0.0;,"
WriteLine cube, "  0.0;0.0;, 0.0;1.0;, 1.0;1.0;, 1.0;0.0;;"
WriteLine cube, " }"
WriteLine cube, "}"
CloseFile cube

Local m = LoadMesh( "cube.x" )
If Not m Then
	WriteLine out, "cube: not loaded"
Else
	WriteLine out, "cube: surfaces=" + CountSurfaces( m )
	For s = 1 To CountSurfaces( m )
		Local surf = GetSurface( m, s )
		WriteLine out, "cube: surface " + s + " verts=" + CountVertices( surf ) + " tris=" + CountTriangles( surf )
		For i = 0 To CountVertices( surf ) - 1
			WriteLine out, "cube: v " + i + " " + VertexX( surf, i ) + " " + VertexY( surf, i ) + " " + VertexZ( surf, i ) + " uv " + VertexU( surf, i ) + " " + VertexV( surf, i ) + " n " + VertexNX( surf, i ) + " " + VertexNY( surf, i ) + " " + VertexNZ( surf, i )
		Next
		For t = 0 To CountTriangles( surf ) - 1
			; the positions, not the vertex numbers: the surface renumbers
			; its own vertices as it is built, so the numbers are the order
			; the surface happened to meet them in, and the positions are
			; what the triangle actually is
			Local a = TriangleVertex( surf, t, 0 )
			Local b = TriangleVertex( surf, t, 1 )
			Local c = TriangleVertex( surf, t, 2 )
			WriteLine out, "cube: t " + t + " " + VertexX( surf, a ) + " " + VertexY( surf, a ) + " " + VertexZ( surf, a ) + " | " + VertexX( surf, b ) + " " + VertexY( surf, b ) + " " + VertexZ( surf, b ) + " | " + VertexX( surf, c ) + " " + VertexY( surf, c ) + " " + VertexZ( surf, c )
		Next
	Next
	WriteLine out, "cube: box " + MeshWidth( m ) + " " + MeshHeight( m ) + " " + MeshDepth( m )
EndIf

;
;	2. a real model, if one was named on the command line
;
Local path$ = Trim( CommandLine() )
If path$ <> "" Then
	Local r = LoadMesh( path$ )
	If Not r Then
		WriteLine out, "model: not loaded"
	Else
		WriteLine out, "model: surfaces=" + CountSurfaces( r )
		For s = 1 To CountSurfaces( r )
			Local rsurf = GetSurface( r, s )
			WriteLine out, "model: surface " + s + " verts=" + CountVertices( rsurf ) + " tris=" + CountTriangles( rsurf )
			Local n = CountVertices( rsurf )
			If n > 4 Then n = 4
			For i = 0 To n - 1
				WriteLine out, "model: v " + i + " " + VertexX( rsurf, i ) + " " + VertexY( rsurf, i ) + " " + VertexZ( rsurf, i ) + " uv " + VertexU( rsurf, i ) + " " + VertexV( rsurf, i ) + " n " + VertexNX( rsurf, i ) + " " + VertexNY( rsurf, i ) + " " + VertexNZ( rsurf, i )
			Next
		Next
		WriteLine out, "model: box " + MeshWidth( r ) + " " + MeshHeight( r ) + " " + MeshDepth( r )
	EndIf
EndIf

CloseFile out
