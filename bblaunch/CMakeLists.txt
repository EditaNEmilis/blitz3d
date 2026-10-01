# bblaunch: Blitz3D.exe, the launcher that checks the machine and starts the IDE.
b3_bin( bblaunch WIN32
	bblaunch.cpp
	bblaunch.ico
	bblaunch.rc
	checkdx.cpp checkdx.h
	checkie.cpp checkie.h )

target_compile_definitions( bblaunch PRIVATE _WINDOWS )
# dxguid for the IID_IDirectDraw* constants checkdx.cpp probes with.
target_link_libraries( bblaunch PRIVATE ${B3_SYSTEM_LIBS} dxguid )

set_target_properties( bblaunch PROPERTIES OUTPUT_NAME "Blitz3D" )
