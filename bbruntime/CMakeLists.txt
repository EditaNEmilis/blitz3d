# bbruntime: the Blitz3D runtime library that compiled user programs link against.
b3_lib( bbruntime bbruntime
	basic.cpp basic.h
	bbaudio.cpp bbaudio.h
	bbbank.cpp bbbank.h
	bbblitz3d.cpp bbblitz3d.h
	bbfilesystem.cpp bbfilesystem.h
	bbgraphics.cpp bbgraphics.h
	bbinput.cpp bbinput.h
	bbmath.cpp bbmath.h
	bbruntime.cpp bbruntime.h
	bbsockets.cpp bbsockets.h
	bbstream.cpp bbstream.h
	bbstring.cpp bbstring.h
	bbsys.cpp bbsys.h
	multiplay.cpp multiplay.h
	multiplay_setup.cpp multiplay_setup.h
	multiplay_setup.rc
	resource.h
	std.cpp std.h
	userlibs.cpp userlibs.h )

target_compile_definitions( bbruntime PRIVATE _LIB )
# dplay.h / dplobby.h come from compat/, since the SDK dropped them.
target_link_libraries( bbruntime PUBLIC config stdutil gxruntime blitz3d b3_compat )
target_link_libraries( bbruntime PRIVATE wsock32 winmm user32 gdi32 advapi32 shell32 )
