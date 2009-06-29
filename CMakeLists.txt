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

message(">>>$ENV{BOOST_ROOT}")
if(NOT BOOST_ROOT AND NOT DEFINED ENV{BOOST_ROOT})
  # nothing
  message(FATAL_ERROR "BOOST_ROOT set neither in cache nor in environment.  No idea where to start looking for our beloved boost.")
endif()


#
#  Include boost headers
#
if (IS_DIRECTORY "$ENV{BOOST_ROOT}")
  message(STATUS "BOOST_ROOT in environment is set and is a directory: trying.")
  set(tmp $ENV{BOOST_ROOT})
else()
  set(tmp ${BOOST_ROOT})
endif()

set(BOOST_ROOT ${tmp}
	       CACHE PATH
	       "Path to boost root (should contain boost/ headers directory")
message(STATUS "BOOST_ROOT = ${BOOST_ROOT}")

if(NOT EXISTS "${BOOST_ROOT}/boost/version.hpp")
  message(FATAL_ERROR "Hrm, boost/version.hpp not found under BOOST_ROOT, cannot continue.")
else()
  message(STATUS "boost/version.hpp found under BOOST_ROOT.  So far so good.")
endif()

include_directories(${BOOST_ROOT})

message(STATUS "Setting BOOST_LIBRARIES_DIR to default \$BOOST_ROOT/build/lib")

set(BOOST_LIBRARIES_DIR "${BOOST_ROOT}/build/lib"
  CACHE PATH
  "Path to directory containing binary boost libraries (e.g. build/lib)")
message(STATUS "BOOST_LIBRARIES_DIR = ${BOOST_LIBRARIES_DIR}")

if(IS_DIRECTORY "${BOOST_LIBRARIES_DIR}")
  message(STATUS "BOOST_LIBRARIES_DIR is a directory.  Trying.")
else()
  message(FATAL_ERROR "BOOST_LIBRARIES_DIR is not a directory.  Cannot continue.")
endif()


find_library(BOOST_PROGRAM_OPTIONS
  boost_program_options-mt-d
  PATHS ${BOOST_LIBRARIES_DIR}
  NO_DEFAULT_PATH)

if (NOT BOOST_PROGRAM_OPTIONS)
  message(SEND_ERROR "*** program options not found at BOOST_ROOT/build/lib")
  message(SEND_ERROR "*** You can set BOOST_LIBRARIES_DIR in your cache if your built") 
  message(SEND_ERROR "*** boost is not found in this default location.") 
  message(SEND_ERROR "*** ") 
  message(SEND_ERROR "*** Note that BOOST_ROOT must contain the boost/ headers directory.") 
endif ()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in 
  ${CMAKE_CURRENT_BINARY_DIR}/config.hpp)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(cmake)

foreach(subdir 
    lisp
    word_count_static
    lex
    )
  add_subdirectory(${subdir})
endforeach()
