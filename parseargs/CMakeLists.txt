cmake_minimum_required(VERSION 3.0.2)
project(parseargs)

set(CMAKE_C_FLAGS "-std=gnu99 -O2")

include_directories("include")

add_library(parseargs STATIC parseargs.c)
