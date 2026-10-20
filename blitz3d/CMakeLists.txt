# blitz3d: scene graph, entities, meshes, terrain, model loaders.
b3_lib( blitz3d blitz3d
	animation.cpp animation.h
	animator.cpp animator.h
	blitz3d.h
	brush.cpp brush.h
	cachedtexture.cpp cachedtexture.h
	camera.cpp camera.h
	collision.cpp collision.h
	entity.cpp entity.h
	frustum.cpp frustum.h
	geom.cpp geom.h
	light.cpp light.h
	listener.cpp listener.h
	loader_3ds.cpp loader_3ds.h
	loader_b3d.cpp loader_b3d.h
	# loader_x.cpp is the original reader. It needs <dxfile.h>, <rmxftmpl.h>
	# and <rmxfguid.h> from the DirectX SDK, which Microsoft no longer
	# distributes (only the End-User Runtime remains), plus d3dxof.dll at
	# run time, which is equally gone. loader_x_native.cpp reads the same
	# format without either: xfile.cpp parses the text and binary encodings
	# of the .x file, and the loader reads the standard templates out of
	# the tree the parser builds. Swap it for loader_x.cpp if you restore
	# the SDK and want the original back.
	loader_x_native.cpp loader_x.h
	md2model.cpp md2model.h
	md2norms.cpp md2norms.h
	md2rep.cpp md2rep.h
	meshcollider.cpp meshcollider.h
	meshloader.cpp meshloader.h
	meshmodel.cpp meshmodel.h
	meshutil.cpp meshutil.h
	mirror.cpp mirror.h
	model.cpp model.h
	object.cpp object.h
	pivot.cpp pivot.h
	planemodel.cpp planemodel.h
	q3bspmodel.cpp q3bspmodel.h
	q3bsprep.cpp
	rendercontext.h
	sprite.cpp sprite.h
	std.cpp std.h
	surface.cpp surface.h
	terrain.cpp terrain.h
	terrainrep.cpp terrainrep.h
	texture.cpp texture.h
	world.cpp world.h
	xfile.cpp xfile.h )

target_compile_definitions( blitz3d PRIVATE _LIB )
# blitz3d/std.h pulls in gxruntime's public header for gxRuntime.
target_include_directories( blitz3d PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../gxruntime" )
target_link_libraries( blitz3d PUBLIC config stdutil gxruntime freeimage )
