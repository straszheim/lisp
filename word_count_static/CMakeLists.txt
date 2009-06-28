##
## Copyright Troy D. Straszheim 2009
##
## Distributed under the Boost Software License, Version 1.0
## See http://www.boost.org/LICENSE_1.0.txt
##

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(word_count_generate 
  word_count_generate.cpp
  )

add_custom_command(OUTPUT word_count_static.hpp
  COMMAND word_count_generate ${CMAKE_CURRENT_BINARY_DIR}/word_count_static.hpp
  )

set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/word_count_static.hpp
  PROPERTIES GENERATED TRUE
  )

add_executable(word_count_static 
  word_count_static.cpp 
  ${CMAKE_CURRENT_BINARY_DIR}/word_count_static.hpp
  )



add_executable(word_count_lexer_generate 
  word_count_lexer_generate.cpp
  )

add_custom_command(OUTPUT word_count_lexer_static.hpp
  COMMAND word_count_lexer_generate ${CMAKE_CURRENT_BINARY_DIR}/word_count_lexer_static.hpp
  )

set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/word_count_lexer_static.hpp
  PROPERTIES GENERATED TRUE)

add_executable(word_count_lexer_static 
  word_count_lexer_static.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/word_count_lexer_static.hpp
  )