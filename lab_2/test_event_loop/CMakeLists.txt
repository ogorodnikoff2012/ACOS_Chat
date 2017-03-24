cmake_minimum_required(VERSION 3.0.2)
project(test_event_loop)

set(CMAKE_C_FLAGS "-std=gnu99 -O2")

find_package(Threads)
include_directories("include")

add_subdirectory("./lib/event_loop")

add_executable(test_event_loop src/test_event_loop.c)
target_link_libraries(test_event_loop event_loop ${CMAKE_THREAD_LIBS_INIT})
