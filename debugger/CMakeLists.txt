# debugger: debugger.dll, the MFC front end the IDE drives while a program runs.
b3_bin( debugger SHARED
	debugger.rc
	debuggerapp.cpp debuggerapp.h
	debugtree.cpp debugtree.h
	mainframe.cpp mainframe.h
	prefs.cpp prefs.h
	sourcefile.cpp sourcefile.h
	stdafx.cpp stdafx.h
	stdutil.cpp
	tabber.cpp tabber.h
	debugger.h )

target_compile_definitions( debugger PRIVATE _WINDOWS _USRDLL _WINDLL DEBUGGER_EXPORTS )
target_link_libraries( debugger PRIVATE ${B3_SYSTEM_LIBS} config stdutil b3_mfc )

set_target_properties( debugger PROPERTIES OUTPUT_NAME "debugger" )
