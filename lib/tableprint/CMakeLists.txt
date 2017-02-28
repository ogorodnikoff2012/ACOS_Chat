cmake_minimum_required(VERSION 3.0.2)
project(tableprint)

set(CMAKE_C_FLAGS "-std=gnu99 -O2")

include_directories("include")

add_library(tableprint STATIC src/tableprint.c)
