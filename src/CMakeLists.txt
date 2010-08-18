##
## Copyright Troy D. Straszheim 2009
##
## Distributed under the Boost Software License, Version 1.0
## See http://www.boost.org/LICENSE_1.0.txt
##

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(lisp 
  main.cpp ops.cpp context.cpp eval.cpp types.cpp
  debug.cpp print.cpp dot.cpp
  grammar.cpp backquote.cpp
  )

if(USE_READLINE)
  target_link_libraries(lisp readline)
endif()

target_link_libraries(lisp ${Boost_LIBRARIES})

