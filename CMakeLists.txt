cmake_minimum_required(VERSION 3.18)
project(pascal2c)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)

add_library(pa lib/parser.c lib/scanner.c lib/token.c)
target_include_directories(pa PRIVATE include)

add_executable(repl utils/repl.c)
target_include_directories(repl PRIVATE include)
target_link_libraries(repl pa)

add_executable(tokens utils/tokens.c)
target_include_directories(tokens PRIVATE include)
target_link_libraries(tokens pa)
