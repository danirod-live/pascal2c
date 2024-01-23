cmake_minimum_required(VERSION 3.18)

add_library(pasta
	parser.c
	parser-common.c
	parser-parameter-list.c
	parser-unsigned-numint.c
	scanner.c
	token.c
)
target_include_directories(pasta PRIVATE ${CMAKE_SOURCE_DIR}/include)
