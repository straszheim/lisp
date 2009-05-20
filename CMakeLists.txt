cmake_minimum_required(VERSION 2.6)

find_path(EXPORTS_DIR 
  exports
  $ENV{BOOST_ROOT}
  NO_DEFAULT_PATH)

message(STATUS "Boost exports found at ${EXPORTS_DIR}")

if (NOT EXPORTS_DIR)
  message(STATUS "exports/*.cmake not found under BOOST_ROOT=$ENV{BOOST_ROOT}")
  message(FATAL_ERROR "BOOST_ROOT has problems")
endif()

file(GLOB BOOST_EXPORTS "${EXPORTS_DIR}/exports/*.cmake")

foreach(import_cmake
    ${BOOST_EXPORTS})
  include(${import_cmake})
endforeach()

list(LENGTH BOOST_EXPORTS NEXPORTS)
message(STATUS "Imported ${NEXPORTS} targets from boost build directory")



add_executable(foo foo.cpp)
target_link_libraries(foo boost_system-mt-shared)