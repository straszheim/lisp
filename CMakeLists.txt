cmake_minimum_required(VERSION 2.6)

if(BOOST_BUILD_DIR)

  find_path(EXPORTS_DIR 
    exports/paths.cmake
    ${BOOST_BUILD_DIR}
    NO_DEFAULT_PATH)

  message(STATUS "Boost exports found at ${EXPORTS_DIR}")

  if (NOT EXPORTS_DIR)
    message(STATUS "exports/*.cmake not found under BOOST_BUILD_DIR=${BOOST_BUILD_DIR}")
    message(FATAL_ERROR "BOOST_BUILD_DIR has problems")
  endif()

  file(GLOB BOOST_EXPORTS "${EXPORTS_DIR}/exports/*.cmake")

  foreach(import_cmake
      ${BOOST_EXPORTS})
    include(${import_cmake})
  endforeach()

  list(LENGTH BOOST_EXPORTS NEXPORTS)
  message(STATUS "Imported ${NEXPORTS} targets from boost build directory")
else()
  find_package(Boost 1.38.0)
  if (NOT Boost_FOUND)
    message(FATAL_ERROR "BOOST_BUILD_DIR not set and boost not found on system")
  endif()
endif()

include_directories(${Boost_INCLUDE_DIR})

add_executable(foo foo.cpp)
target_link_libraries(foo boost_system-mt-shared)