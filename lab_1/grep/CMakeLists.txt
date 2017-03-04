cmake_minimum_required(VERSION 3.0.2)
project(grep)

set(CMAKE_C_FLAGS "-std=gnu99 -O2 -D_GNU_SOURCE")

add_executable(grep src/grep.c)
