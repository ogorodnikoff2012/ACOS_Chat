cmake_minimum_required(VERSION 3.0.2)
project(chat)

set(CMAKE_C_FLAGS "-std=gnu99 -D_GNU_SOURCE -g -O0 -DDEBUG")

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules/")

find_package(Threads)
find_package(SQLite3)
include("./cmake_modules/load_ncurses.cmake")

include_directories("include" ${SQLITE3_INCLUDE_DIR})

add_subdirectory("./lib/event_loop")
add_subdirectory("./lib/ts_vector")
add_subdirectory("./lib/xenon_md5")

add_executable(client
        src/client/events/display_parsed_message_job.c
        src/client/events/readinput_event.c
        src/client/gui.c
        src/client/main.c
        src/client/misc.c
        src/client/parsed_message.c
        src/common/logger.c
        src/common/message.c
        src/common/pascal_string.c
        src/client/events/send_message_to_server_job.c src/client/controller.c src/client/listener.c src/client/events/hangup_event.c src/client/events/kick_event.c src/client/events/list_event.c src/client/events/status_event.c)
target_link_libraries(client event_loop ts_vector ${CMAKE_THREAD_LIBS_INIT} ${CURSES_LIBRARIES})

add_executable(server
        src/common/logger.c
        src/common/message.c
        src/common/pascal_string.c
        src/server/controller.c
        src/server/conn_mgr.c
        src/common/connection.c
        src/server/db.c
        src/server/events/broadcast_message_event.c
        src/server/events/change_worker_cnt_event.c
        src/server/events/close_connection_event.c
        src/server/events/input_msg_event.c
        src/server/events/process_message_job.c
        src/server/events/send_message_job.c
        src/server/events/worker_exit_event.c
        src/server/listener.c
        src/server/main.c
        src/server/misc.c
        src/server/worker.c
        )
target_link_libraries(server event_loop ts_vector xenon_md5 ${CMAKE_THREAD_LIBS_INIT} ${SQLite3_LIBRARIES})
