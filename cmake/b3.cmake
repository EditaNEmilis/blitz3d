# Helpers shared by the Blitz3D project files.

# b3_require(<path> <what>)
# Fail with a readable message instead of letting a missing dependency turn
# into a wall of compiler errors later on.
function( b3_require path what )
	if( NOT EXISTS "${path}" )
		message( FATAL_ERROR "Cannot find ${what} at ${path}" )
	endif()
endfunction()

# b3_common(<target>)
# Applies what every Blitz3D .dsp shared: /MT, /W3 and /D "PRO", which is the
# full release build rather than the demo or educational edition.
#
# /Gz is not optional. The .dsp files all carried it, and it is the difference
# between a working runtime and a subtly corrupt one: blitzcc's x86 backend emits
# a bare "call _bbStrConst" with no stack cleanup, because every runtime function
# is __stdcall under that default. Built as __cdecl (MSVC's C++ default on x86),
# each nested call leaves its arguments behind, the frame that Print's argument
# list was built in slides away, and __bbStrConcat ends up handed the integer 5
# where the first string operand belongs.
function( b3_common target )
	target_compile_definitions( ${target} PRIVATE WIN32 NDEBUG _MBCS PRO )
	target_compile_options( ${target} PRIVATE /W3 /Oi /Gy /Gz )
	# Every .rc in the tree includes <afxres.h>, so the MFC headers have to be
	# reachable from resource compilation even in the non-MFC projects.
	target_include_directories( ${target} PRIVATE "${CMAKE_ATLMFC_INCLUDE_DIRECTORIES}" )
	set_target_properties( ${target} PROPERTIES
		MSVC_RUNTIME_LIBRARY "MultiThreaded"
		DEBUG_POSTFIX "" )
endfunction()

# b3_lib(<name> <out.lib> SOURCES ... )
# One of the static libraries from the workspace. <out.lib> is the file name the
# dependent projects link against, i.e. the plain project name.
function( b3_lib name outlib )
	add_library( ${name} STATIC ${ARGN} )
	b3_common( ${name} )
	set_target_properties( ${name} PROPERTIES
		OUTPUT_NAME "${outlib}"
		ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib" )
endfunction()

# b3_o1(<target>)
# Pin a target to /O1. The .dsp files built the code-generating chain (compiler,
# linker, blitzcc) at /O1 while the runtime used /O2; keeping that split means the
# port produces the same code the originals did.
function( b3_o1 target )
	target_compile_options( ${target} PRIVATE /O1 )
endfunction()

# b3_bin(<name> <WIN32|CONSOLE|SHARED> SOURCES ... )
# One of the shipped .exe/.dll targets. The per-project CMakeLists add the
# defines and libraries that the matching .dsp carried.
#
# SHARED has to go through add_library: add_executable has no SHARED keyword.
function( b3_bin name type )
	# SHARED has to go through add_library: add_executable has no SHARED keyword.
	# CONSOLE is add_executable's default, so it is spelled as no keyword at all.
	if( type STREQUAL "SHARED" )
		add_library( ${name} SHARED ${ARGN} )
	elseif( type STREQUAL "CONSOLE" )
		add_executable( ${name} ${ARGN} )
	else()
		add_executable( ${name} ${type} ${ARGN} )
	endif()
	b3_common( ${name} )
	set_target_properties( ${name} PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
		LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )
endfunction()

# b3_stage(<target> <dest-dir>)
# Copy the built binary into the ship layout. A custom target rather than
# add_custom_command(OUTPUT ...), because the targets live in subdirectories and
# CMake does not resolve target generator expressions in a cross-directory OUTPUT.
function( b3_stage target dest )
	add_custom_target( "stage_${target}" ALL
		COMMAND ${CMAKE_COMMAND} -E make_directory "${dest}"
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"$<TARGET_FILE:${target}>" "${dest}/$<TARGET_FILE_NAME:${target}>"
		DEPENDS ${target}
		COMMENT "Staging $<TARGET_FILE_NAME:${target}>" )
endfunction()
