# linker_dll: the thin DLL wrapper the IDE loads to get at the linker.
b3_bin( linker_dll SHARED
	linker_dll.cpp )

target_compile_definitions( linker_dll PRIVATE _WINDOWS _USRDLL LINKER_DLL_EXPORTS )
target_link_libraries( linker_dll PRIVATE ${B3_SYSTEM_LIBS} linker )

set_target_properties( linker_dll PROPERTIES OUTPUT_NAME "linker" )
