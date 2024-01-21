cmake_minimum_required(VERSION 3.18)

add_library(pa parser.c scanner.c token.c)
target_include_directories(pa PRIVATE ${CMAKE_SOURCE_DIR}/include)
