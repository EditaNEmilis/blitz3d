# stdutil: small helpers shared by the compiler, the linker and the runtime.
b3_lib( stdutil stdutil
	stdutil.cpp
	stdutil.h )

target_link_libraries( stdutil PUBLIC config )
