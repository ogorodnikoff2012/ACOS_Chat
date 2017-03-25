cmake_minimum_required(VERSION 3.0.2)
project(test_ts_map)

set(CMAKE_C_FLAGS "-std=gnu99 -g -O0")

find_package(Threads)
include_directories("include")

add_subdirectory("./lib/ts_map")

add_executable(test_ts_map src/test_ts_map.c)
target_link_libraries(test_ts_map ts_map ${CMAKE_THREAD_LIBS_INIT})
