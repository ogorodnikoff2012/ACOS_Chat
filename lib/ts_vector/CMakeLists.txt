cmake_minimum_required(VERSION 3.0.2)
project(ts_vector)

set(CMAKE_C_FLAGS "-std=gnu99 -O2 ${CMAKE_C_FLAGS}")

if (TS_VECTOR_NO_THREADS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DTS_NO_THREADS")
elseif()
    find_package(Threads)
endif()
include_directories("include")

add_library(ts_vector STATIC src/ts_vector.c)
if (NOT TS_VECTOR_NO_THREADS)
    target_link_libraries(ts_vector ${CMAKE_THREAD_LIBS_INIT})
endif()