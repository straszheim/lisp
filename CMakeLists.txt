##
## Copyright Troy D. Straszheim 2009
##
## Distributed under the Boost Software License, Version 1.0
## See http://www.boost.org/LICENSE_1.0.txt
##
cmake_minimum_required(VERSION 2.6)
include(CheckIncludeFiles)
check_include_files("readline/readline.h;readline/history.h" USE_READLINE)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build debug by default" FORCE)
set(CMAKE_CXX_FLAGS "-Wall")

if(USE_READLINE)
  message(STATUS "readline/readline.h found, enabling readline")
endif()

#
# find_package(Boost 1.41.0 COMPONENTS program_options NO_MODULE)
#
include("/home/troy/Projects/boost/cmake/build/lib/Exports.cmake")
include_directories("/home/troy/Projects/boost/cmake/src")


configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in 
  ${CMAKE_CURRENT_BINARY_DIR}/config.hpp)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# add_subdirectory(cmake)

add_subdirectory(src)
