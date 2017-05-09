cmake_minimum_required(VERSION 3.0.2)
project(chat)

set(CMAKE_C_FLAGS "-std=gnu99 -g -O0 -DDEBUG")

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules/")

find_package(Threads)
find_package(SQLite3)
include("./cmake_modules/load_ncurses.cmake")

include_directories("include" ${SQLITE3_INCLUDE_DIR})

add_subdirectory("./lib/event_loop")
add_subdirectory("./lib/ts_vector")
add_subdirectory("./lib/xenon_md5")

add_executable(client
        src/client/main.c
        src/client/gui.c)
target_link_libraries(client event_loop ${CMAKE_THREAD_LIBS_INIT} ${CURSES_LIBRARIES})

add_executable(server
        src/server/main.c
        src/server/controller.c
        src/server/worker.c
        src/server/listener.c
        src/logger.c
        src/server/conn_mgr.c)
target_link_libraries(server event_loop ts_vector xenon_md5 ${CMAKE_THREAD_LIBS_INIT} ${SQLITE3_LIBRARIES})
