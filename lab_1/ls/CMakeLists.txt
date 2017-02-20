cmake_minimum_required(VERSION 3.0.2)
project(ls)

set(CMAKE_C_FLAGS "-std=gnu11 -O2")

include_directories("include")

add_subdirectory("lib/parseargs")

add_executable(ls src/ls.c)
target_link_libraries(ls parseargs)
