# blitzide: ide.exe, the MFC IDE.
b3_bin( ide WIN32
	about.cpp about.h
	blitzide.cpp blitzide.h
	blitzide.rc
	editor.cpp editor.h
	funclist.cpp funclist.h
	htmlhelp.cpp htmlhelp.h
	libs.cpp libs.h
	mainframe.cpp mainframe.h
	prefs.cpp prefs.h
	stdafx.cpp stdafx.h
	tabber.cpp tabber.h
	b3dlogo.bmp
	"IDE (logo, no code) 32.ico" )

target_compile_definitions( ide PRIVATE _WINDOWS )
target_link_libraries( ide PRIVATE
	${B3_SYSTEM_LIBS}
	config stdutil compiler linker freeimage b3_mfc )

set_target_properties( ide PROPERTIES OUTPUT_NAME "ide" )
