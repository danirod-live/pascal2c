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
	scanner.c
	token.c
)
target_include_directories(pasta PRIVATE ${CMAKE_SOURCE_DIR}/include)
