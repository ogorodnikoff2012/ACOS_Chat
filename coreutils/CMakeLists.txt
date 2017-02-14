cmake_minimum_required(VERSION 3.0.2)
project(ls)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -O2 -g -D_GLIBCXX_DEBUG -Wall -Wextra -Werror")

include_directories(include)
set(SOURCE_FILES )
add_executable(ls src/ls.c ${SOURCE_FILES})
