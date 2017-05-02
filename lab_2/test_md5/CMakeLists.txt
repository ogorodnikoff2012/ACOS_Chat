cmake_minimum_required(VERSION 3.0.2)
project(test_md5)

set(CMAKE_C_FLAGS "-std=gnu99 -g -O0")

include_directories("include")

add_subdirectory("./lib/xenon_md5")

add_executable(test_md5 src/test_md5.c)
target_link_libraries(test_md5 xenon_md5)
