cmake_minimum_required(VERSION 3.0.2)
project(ts_queue)

set(CMAKE_C_FLAGS "-std=gnu99 -O2 ${CMAKE_C_FLAGS}")

find_package(Threads)
include_directories("include")

add_library(ts_map STATIC src/ts_map.c)
target_link_libraries(ts_map ${CMAKE_THREAD_LIBS_INIT})
