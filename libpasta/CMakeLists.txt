cmake_minimum_required(VERSION 3.18)

add_library(pasta
	parser.c
	parser-block.c
	parser-common.c
	parser-constant.c
	parser-expression.c
	parser-field-list.c
	parser-parameter-list.c
	parser-program.c
	parser-simple-type.c
	parser-simple.c
	parser-statement.c
	parser-type.c
	parser-variable.c
	parser2-block.c
	parser2-common.c
	parser2-simple.c
	parser2-variable.c
	parser2-constant.c
	parser2-expression.c
	parser2-field-list.c
	parser2-parameter-list.c
	parser2-program.c
	parser2-statement.c
	parser2-type.c
	scanner.c
	token.c
)
target_include_directories(pasta PRIVATE ${CMAKE_SOURCE_DIR}/include)
