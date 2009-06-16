cmake_minimum_required(VERSION 2.6)

include_directories("/home/troy/Projects/boost/src/")
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_executable(lisp 
  main.cpp ops.cpp context.cpp eval.cpp types.cpp
  debug.cpp print.cpp
  )
# add_executable(tst calc4.cpp)
