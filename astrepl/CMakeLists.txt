cmake_minimum_required(VERSION 3.18)

add_executable(astrepl
	repl.c
)
target_include_directories(astrepl PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(astrepl pasta)
