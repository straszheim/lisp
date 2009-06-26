##
## Copyright Troy D. Straszheim 2009
##
## Distributed under the Boost Software License, Version 1.0
## See http://www.boost.org/LICENSE_1.0.txt
##
cmake_minimum_required(VERSION 2.6)

find_path(READLINE_H_PATH readline/readline.h)
if(READLINE_H_PATH)
  message("readline/readline.h found at ${READLINE_H_PATH}, enabling readline")
endif()

if(READLINE_H_PATH)
  option(USE_READLINE "Use gnu readline" ON)
else()
  option(USE_READLINE "Use gnu readline" OFF)
endif()

if(USE_READLINE)
  add_definitions(-DHAVE_READLINE_H)
endif()
    

#
#  Include boost headers
#
set(BOOST_ROOT "BOOST_ROOT-NOTFOUND"
	       CACHE PATH
	       "Path to boost root (should contain boost/ headers directory")
include_directories(${BOOST_ROOT})

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

