cmake_minimum_required(VERSION 3.0.2)
project(xenon_md5)

set(CMAKE_C_FLAGS "-std=gnu99 -O2 ${CMAKE_C_FLAGS}")

include_directories("include")

add_library(xenon_md5 STATIC src/xenon_md5.c)
target_link_libraries(xenon_md5 m) # m is for math :-)
