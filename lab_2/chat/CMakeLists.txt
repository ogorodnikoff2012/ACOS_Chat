cmake_minimum_required(VERSION 3.0.2)
project(chat)

set(CMAKE_C_FLAGS "-std=gnu99 -g -O0")

find_package(Threads)
include("load_ncurses.cmake")

include_directories("include")

add_subdirectory("./lib/event_loop")

add_executable(client src/client/main.c src/client/gui.c)
target_link_libraries(client event_loop ${CMAKE_THREAD_LIBS_INIT} ${CURSES_LIBRARIES})
# add_executable(server src/server/main.c)
# target_link_libraries(server event_loop ${CMAKE_THREAD_LIBS_INIT} ${CURSES_LIBRARIES})
