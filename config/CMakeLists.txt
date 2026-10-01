# config: header only, the shared version/edition bits every other project reads.
# An interface target, because there is nothing to compile.
add_library( config INTERFACE )
target_include_directories( config INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}" )
