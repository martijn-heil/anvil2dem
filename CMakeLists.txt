cmake_minimum_required(VERSION 2.5)
project(testproject C)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -pedantic -ggdb -ftrapv -pipe -Wall -Wextra -Wno-unused-function -D_POSIX_C_SOURCE -D_REENTRANT -D_POSIX_C_SOURCE -D_GNU_SOURCE -Wl,--no-as-needed -lz")

include_directories(
    src/,
    lib/
)

file(GLOB_RECURSE SOURCES src/*)
file(GLOB_RECURSE HEADERS src/*.h)
file(GLOB_RECURSE LIBRARY_SOURCES lib/*)
add_executable(testproject ${SOURCES} ${LIBRARY_SOURCES})
