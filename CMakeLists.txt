cmake_minimum_required(VERSION 3.18)
project(pascal2c)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)

add_subdirectory(libpasta)

add_executable(repl utils/repl.c)
target_include_directories(repl PRIVATE include)
target_link_libraries(repl pasta)

add_executable(tokens utils/tokens.c)
target_include_directories(tokens PRIVATE include)
target_link_libraries(tokens pasta)
