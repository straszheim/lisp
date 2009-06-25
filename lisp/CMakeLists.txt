##
## Copyright Troy D. Straszheim 2009
##
## Distributed under the Boost Software License, Version 1.0
## See http://www.boost.org/LICENSE_1.0.txt
##
cmake_minimum_required(VERSION 2.6)

option(USE_READLINE "Use gnu readline" OFF)

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

