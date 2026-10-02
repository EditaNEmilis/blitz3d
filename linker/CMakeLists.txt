# linker: turns compiled Blitz3D objects into executables.
b3_lib( linker linker
	image_util.cpp image_util.h
	linker.cpp linker.h
	std.cpp std.h )

target_compile_definitions( linker PRIVATE _LIB )
target_link_libraries( linker PUBLIC config stdutil )
