try_compile(VERSION_CHECK 
  ${CMAKE_CURRENT_BINARY_DIR}/version_check 
  ${CMAKE_CURRENT_SOURCE_DIR}/version_check 
  version_check # project name.. what for?
  CMAKE_FLAGS -DBOOST_ROOT:PATH=${BOOST_ROOT}
  )

if (VERSION_CHECK)
  message(STATUS "Able to compile test program that uses <boost/version.hpp>.  Wonderful.")
  execute_process(COMMAND ${CMAKE_CURRENT_BINARY_DIR}/version_check/main
    OUTPUT_VARIABLE VERSION_CHECK_OUT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  message(STATUS "Test program reports:  ${VERSION_CHECK_OUT}")
else()
  message(SEND_ERROR "Unable to compile program that uses <boost/version.hpp> (found via BOOST_ROOT)")
endif()



try_compile(PROGRAM_OPTIONS_CHECK 
  ${CMAKE_CURRENT_BINARY_DIR}/program_options_check 
  ${CMAKE_CURRENT_SOURCE_DIR}/program_options_check
  program_options_check # project name.. what for?
  CMAKE_FLAGS 
  -DBOOST_ROOT:PATH=${BOOST_ROOT}
  -DBOOST_PROGRAM_OPTIONS:PATH=${BOOST_PROGRAM_OPTIONS}
  )

if (PROGRAM_OPTIONS_CHECK)
  message(STATUS "Able to compile and link test program that uses boost program_options.  Fantastic.")
  execute_process(COMMAND ${CMAKE_CURRENT_BINARY_DIR}/program_options_check/main
    OUTPUT_VARIABLE PROGRAM_OPTIONS_CHECK_OUT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  message(STATUS "Test program reports:  ${PROGRAM_OPTIONS_CHECK_OUT}")
else()
  message(SEND_ERROR "Unable to compile and link test program that uses boost program_options.")
endif()


