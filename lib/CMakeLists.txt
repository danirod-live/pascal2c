cmake_minimum_required(VERSION 3.18)

add_library(pa parser.c parser-parameter-list.c scanner.c token.c)
target_include_directories(pa PRIVATE ${CMAKE_SOURCE_DIR}/include)
