cmake_minimum_required(VERSION 2.5)
project(anvil2dem C)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -pedantic -ggdb -ftrapv -pipe -Wall -Wextra -Wno-unused-function -D_POSIX_C_SOURCE -D_REENTRANT -D_POSIX_C_SOURCE -D_GNU_SOURCE -Wl,--no-as-needed -lz -ltiff -lgeotiff")

include_directories(
    src/,
    lib/
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")

# cmocka
option(ENABLE_TESTS "Perform unit tests after build" OFF)
if (ENABLE_TESTS)
  find_package(CMocka CONFIG REQUIRED)
  include(AddCMockaTest)
  include(AddMockedTest)
  add_subdirectory(test)
  enable_testing()
endif(ENABLE_TESTS)

file(GLOB_RECURSE SOURCES src/*)
file(GLOB_RECURSE HEADERS src/*.h)
file(GLOB_RECURSE LIBRARY_SOURCES lib/*)
add_executable(anvil2dem ${SOURCES} ${LIBRARY_SOURCES})
