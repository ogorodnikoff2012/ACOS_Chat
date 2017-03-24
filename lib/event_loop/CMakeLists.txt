cmake_minimum_required(VERSION 3.0.2)
project(event_loop)

set(CMAKE_C_FLAGS "-std=gnu99 -O2")

find_package(Threads)
include_directories("include")

add_subdirectory("./lib/ts_queue")
add_subdirectory("./lib/ts_map")

add_library(event_loop STATIC src/event_loop.c)
target_link_libraries(event_loop ts_queue ts_map ${CMAKE_THREAD_LIBS_INIT})
