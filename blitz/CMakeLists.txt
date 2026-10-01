# blitz: blitzcc.exe, the command line Blitz compiler.
b3_bin( blitzcc CONSOLE
	libs.cpp libs.h
	main.cpp )

target_compile_definitions( blitzcc PRIVATE _CONSOLE )
b3_o1( blitzcc )
target_link_libraries( blitzcc PRIVATE
	${B3_SYSTEM_LIBS}
	compiler linker config stdutil blitz3d gxruntime bbruntime )

set_target_properties( blitzcc PROPERTIES OUTPUT_NAME "blitzcc" ENABLE_EXPORTS ON )
