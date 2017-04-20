cmake_minimum_required(VERSION 3.0.2)
project(ts_vector)

set(CMAKE_C_FLAGS "-std=gnu99 -O2 ${CMAKE_C_FLAGS}")

find_package(Threads)
include_directories("include")

add_library(ts_vector STATIC src/ts_vector.c)
target_link_libraries(ts_vector ${CMAKE_THREAD_LIBS_INIT})
