# Blitz3D - modern (MSVC 2019/2022, 32-bit) CMake build.
#
# Replaces the MSVC 6.0 .dsp/.dsw workspace, which needs a compiler that no
# longer exists and an MFC SKU that Community no longer pairs with. Every
# project below mirrors one .dsp from blitz3d.dsw, including its source list,
# preprocessor defines, link libraries and output path.
#
# Configure and build from a 32-bit developer shell:
#   tools\build.cmd
#
# or by hand:
#   call "%VSINSTALLDIR%VC\Auxiliary\Build\vcvarsall.bat" x86
#   cmake -S . -B build -G Ninja
#   cmake --build build

cmake_minimum_required( VERSION 3.20 )

project( Blitz3D VERSION 1.108 LANGUAGES CXX RC )

if( NOT MSVC )
	message( FATAL_ERROR "Blitz3D only builds with MSVC." )
endif()

if( NOT CMAKE_SIZEOF_VOID_P EQUAL 4 )
	message( FATAL_ERROR
		"Blitz3D is a 32-bit application. Configure from an x86 developer shell "
		"(vcvarsall.bat x86) or pass -A Win32 to the Visual Studio generator." )
endif()

set( CMAKE_CXX_STANDARD 14 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
# VC6 projects all used /MT; the release DLLs have to stay self contained.
set( CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded" )

list( APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" )
include( b3 )

# ---------------------------------------------------------------------------
# Third party trees dropped in next to the sources, per README steps 1 and 2.
# ---------------------------------------------------------------------------
set( FREEIMAGE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/freeimage241" )
set( FMOD_DIR      "${CMAKE_CURRENT_SOURCE_DIR}/fmodapi375win" )

if( NOT EXISTS "${FMOD_DIR}/api/inc/fmod.h" )
	message( FATAL_ERROR
		"fmodapi375win not found. See README.TXT step 2; the archive is no longer "
		"served by fmod.org and has to be placed at ${FMOD_DIR}." )
endif()

add_library( freeimage STATIC IMPORTED GLOBAL )
set_target_properties( freeimage PROPERTIES
	IMPORTED_LOCATION  "${FREEIMAGE_DIR}/Source/FreeImageLib/Release/FreeImage.lib"
	IMPORTED_IMPLIB   "${FREEIMAGE_DIR}/Source/FreeImageLib/Release/FreeImage.lib" )
target_include_directories( freeimage INTERFACE "${FREEIMAGE_DIR}/Source" )
# The shipped FreeImage.lib was compiled by VC6, so its objects call the old
# underscore-prefixed stdio entry points (_sprintf, _fprintf, _sscanf, __iob).
# MSVC still ships those names, but only in this side library.
find_library( B3_LEGACY_STDIO_LIB NAMES legacy_stdio_definitions )
if( B3_LEGACY_STDIO_LIB )
	target_link_libraries( freeimage INTERFACE "${B3_LEGACY_STDIO_LIB}" )
endif()

add_library( fmod STATIC IMPORTED GLOBAL )
set_target_properties( fmod PROPERTIES
	IMPORTED_LOCATION  "${FMOD_DIR}/api/lib/fmodvc.lib"
	IMPORTED_IMPLIB   "${FMOD_DIR}/api/lib/fmodvc.lib" )
target_include_directories( fmod INTERFACE "${FMOD_DIR}/api/inc" )

# DirectPlay 4 headers, which the Windows SDK dropped along with dplayx.dll.
add_library( b3_compat INTERFACE )
target_include_directories( b3_compat INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/compat" )

# Static MFC for the IDE and the debugger, and afxres.h for every .rc.
if( NOT CMAKE_ATLMFC_INCLUDE_DIRECTORIES )
	# cl.exe lives in <toolset>/bin/Host<arch>/<arch>/; MFC is at <toolset>/atlmfc.
	get_filename_component( _b3_cl "${CMAKE_CXX_COMPILER}" DIRECTORY )
	string( REGEX REPLACE "/bin/.*$" "" _b3_toolset "${_b3_cl}" )
	set( CMAKE_ATLMFC_INCLUDE_DIRECTORIES "${_b3_toolset}/atlmfc/include" )
	set( CMAKE_ATLMFC_LIBRARY_DIRECTORY "${_b3_toolset}/atlmfc/lib/x86" )
endif()
b3_require( "${CMAKE_ATLMFC_INCLUDE_DIRECTORIES}/afxwin.h" "MFC headers" )

# There is no plain mfc.lib any more, and the replacements are named in a way
# that actively misleads. The shape of it:
#   mfc140.lib / MFCM140.lib  import libraries for mfc140.dll (shared MFC)
#   mfc140u.lib                static MFC, Unicode
#   mfcs140.lib                shared-link glue; its dllmodul.obj wants the
#                              shared-MFC-only entry points and so fails against
#                              a static build
#   nafxcw.lib                 static MFC, MBCS - this is the one, and it carries
#                              its own _DllMain@12 so a DLL project needs nothing else
# Blitz3D is _MBCS and never defines UNICODE, so nafxcw is the right flavour.
b3_require( "${CMAKE_ATLMFC_LIBRARY_DIRECTORY}/nafxcw.lib" "static MBCS MFC (nafxcw.lib)" )
set( B3_MFC_LIB nafxcw )
find_library( B3_ATL_LIB NAMES atls HINTS "${CMAKE_ATLMFC_LIBRARY_DIRECTORY}" )

add_library( b3_mfc INTERFACE )
target_include_directories( b3_mfc INTERFACE "${CMAKE_ATLMFC_INCLUDE_DIRECTORIES}" )
target_link_libraries( b3_mfc INTERFACE "${B3_MFC_LIB}" "${B3_ATL_LIB}" )

# The Win32 import libraries the original .dsp files linked, minus the ones the
# SDK no longer ships and that nothing in the tree still calls into:
#   dinput.lib   -> dinput8.lib, gxruntime now uses the DirectInput 8 entry point
#   dplayx.lib   -> nothing; dplayx.dll does not exist on Windows 10/11
#   d3dxof.lib   -> nothing; blitz3d/loader_x.cpp uses <dxfile.h> instead
set( B3_SYSTEM_LIBS
	kernel32 user32 gdi32 winspool comdlg32 advapi32 shell32
	ole32 oleaut32 uuid odbc32 odbccp32 winmm )

# ---------------------------------------------------------------------------
# The workspace, in dependency order.
# ---------------------------------------------------------------------------
add_subdirectory( config )
add_subdirectory( stdutil )
add_subdirectory( compiler )
add_subdirectory( linker )
add_subdirectory( linker_dll )
add_subdirectory( blitz3d )
add_subdirectory( gxruntime )
add_subdirectory( bbruntime )
add_subdirectory( bbruntime_dll )
add_subdirectory( blitz )
add_subdirectory( debugger )
add_subdirectory( blitzide )
add_subdirectory( bblaunch )

# ---------------------------------------------------------------------------
# Ship layout. README steps 5 and 6: binaries land in _release, fmod.dll is
# copied next to runtime.dll so Blitz3D can load it.
# ---------------------------------------------------------------------------
set( B3_RELEASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/_release" )
set( B3_BIN_DIR "${B3_RELEASE_DIR}/bin" )

b3_stage( blitzcc    "${B3_BIN_DIR}" )
b3_stage( runtime    "${B3_BIN_DIR}" )
b3_stage( linker_dll "${B3_BIN_DIR}" )
b3_stage( debugger   "${B3_BIN_DIR}" )
b3_stage( ide        "${B3_BIN_DIR}" )
b3_stage( bblaunch   "${B3_RELEASE_DIR}" )

# blitzcc.exe drives the linker and runtime by bare name, resolved through the
# blitzpath environment variable that bblaunch sets to the app directory.
add_custom_command( OUTPUT "${B3_BIN_DIR}/fmod.dll"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${FMOD_DIR}/api/fmod.dll" "${B3_BIN_DIR}/fmod.dll"
	DEPENDS "${FMOD_DIR}/api/fmod.dll"
	COMMENT "Staging fmod.dll into _release/bin" )
add_custom_target( stage_fmod ALL DEPENDS "${B3_BIN_DIR}/fmod.dll" )
add_dependencies( stage_fmod blitzcc runtime linker_dll debugger ide bblaunch )
