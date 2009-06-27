##
## Copyright Troy D. Straszheim 2009
##
## Distributed under the Boost Software License, Version 1.0
## See http://www.boost.org/LICENSE_1.0.txt
##
cmake_minimum_required(VERSION 2.6)
include(CheckIncludeFiles)
check_include_files("readline/readline.h;readline/history.h" USE_READLINE)

set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build debug by default" FORCE)

if(USE_READLINE)
  message(STATUS "readline/readline.h found, enabling readline")
endif()

#
#  Include boost headers
#
set(BOOST_ROOT "BOOST_ROOT-NOTFOUND"
	       CACHE PATH
	       "Path to boost root (should contain boost/ headers directory")
include_directories(${BOOST_ROOT})

set(BOOST_LIBRARIES_DIR "BOOST_LIBRARIES_DIR-NOTFOUND"
  CACHE PATH
  "Path to directory containing boost libraries")

find_library(BOOST_PROGRAM_OPTIONS
  boost_program_options-mt-d
  PATHS ${BOOST_LIBRARIES_DIR}
  NO_DEFAULT_PATH)

if (NOT BOOST_PROGRAM_OPTIONS)
  message(FATAL_ERROR "Boost program options not found at ${BOOST_LIBRARIES_DIR}.")
endif ()


#
#  Put this directory in include path
#
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

#
#  Compile all the .cpps
#
add_executable(lisp 
  main.cpp ops.cpp context.cpp eval.cpp types.cpp
  debug.cpp print.cpp dot.cpp
  grammar.cpp backquote.cpp
  )

if(USE_READLINE)
  target_link_libraries(lisp readline)
endif()

target_link_libraries(lisp ${BOOST_PROGRAM_OPTIONS})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in 
  ${CMAKE_CURRENT_BINARY_DIR}/config.hpp)
include_directories(${CMAKE_CURRENT_BINARY_DIR})