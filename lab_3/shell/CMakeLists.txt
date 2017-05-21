cmake_minimum_required(VERSION 3.0.2)

project(Smash)

set(CMAKE_C_FLAGS "-std=gnu99 -D_GNU_SOURCE -g -O0 -DDEBUG")
set(TS_MAP_NO_THREADS)
set(TS_VECTOR_NO_THREADS)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules/")

find_package(Readline)

include_directories("include")

add_subdirectory("lib/ts_map")
add_subdirectory("lib/ts_vector")

add_executable(smash src/main.c src/lexer.c src/parser.c src/string_buffer.c src/command.c src/runner.c src/chdir.c)

target_link_libraries(smash ts_map ts_vector ${Readline_LIBRARY})
