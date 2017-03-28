cmake_minimum_required(VERSION 3.0.2)
project(test_ncurses_and_sockets)

set(CMAKE_C_FLAGS "-std=gnu99 -O2") # -g -O0")

find_package(Threads)
include("load_ncurses.cmake")

include_directories("include")

add_subdirectory("./lib/event_loop")

add_executable(prog src/main.c)
target_link_libraries(prog event_loop ${CMAKE_THREAD_LIBS_INIT} ${CURSES_LIBRARIES})
