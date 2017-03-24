cmake_minimum_required(VERSION 3.0.2)
project(ts_queue)

set(CMAKE_C_FLAGS "-std=gnu99 -O2")

find_package(Threads)
include_directories("include")

add_library(ts_queue STATIC src/ts_queue.c)
target_link_libraries(ts_queue ${CMAKE_THREAD_LIBS_INIT})
