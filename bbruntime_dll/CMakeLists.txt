# bbruntime_dll: runtime.dll, the DLL that user programs and the IDE load.
b3_bin( runtime SHARED
	bbruntime_dll.cpp
	bbruntime_dll.h
	bbruntime_dll.rc
	bbexe.ico
	resource.h )

target_compile_definitions( runtime PRIVATE _WINDOWS _USRDLL BBRUNTIME_DLL_EXPORTS )
target_link_libraries( runtime PRIVATE
	${B3_SYSTEM_LIBS}
	bbruntime blitz3d gxruntime freeimage fmod
	wsock32 amstrmid winmm ddraw dinput8 dsound ole32 uuid dxguid )

set_target_properties( runtime PROPERTIES OUTPUT_NAME "runtime" )
