# compiler: the Blitz Basic parser, semantic analyser and x86 code generator.
b3_lib( compiler compiler
	decl.cpp decl.h
	declnode.cpp declnode.h
	environ.cpp environ.h
	exprnode.cpp exprnode.h
	label.h
	node.cpp node.h
	nodes.h
	parser.cpp parser.h
	prognode.cpp prognode.h
	std.cpp std.h
	stmtnode.cpp stmtnode.h
	toker.cpp toker.h
	type.cpp type.h
	varnode.cpp varnode.h
	codegen.h
	ex.h
	assem.h
	assem_x86/asm_insts.cpp
	assem_x86/assem_x86.cpp
	assem_x86/assem_x86.h
	assem_x86/insts.h
	assem_x86/operand.cpp
	assem_x86/operand.h
	codegen_x86/codegen_x86.cpp
	codegen_x86/codegen_x86.h
	codegen_x86/tile.cpp
	codegen_x86/tile.h )

target_compile_definitions( compiler PRIVATE _LIB )
target_link_libraries( compiler PUBLIC config stdutil )
