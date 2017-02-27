cmake_minimum_required(VERSION 3.0.2)
project(top)

set(CMAKE_C_FLAGS "-std=gnu99 -O2")

include_directories("include")

add_subdirectory("lib/parseargs")

add_executable(top src/top.c)
target_link_libraries(top parseargs)
