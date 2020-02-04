project(TestLib)

cmake_minimum_required(VERSION 2.8.12)
set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

#Use the default Julia to find the CxxWrap path
if(NOT DEFINED JL_CXX_WRAP_PATH)
  execute_process(COMMAND julia "-e" "using CxxWrap; print(CxxWrap.CxxWrapCore.prefix_path())"
    RESULT_VARIABLE julia_cxxwrap_result
    OUTPUT_VARIABLE JL_CXX_WRAP_PATH)

  if(NOT "${julia_cxxwrap_result}" STREQUAL "0")
      message(FATAL_ERROR "Could not find Julia's CxxWrap Location. Please set JL_CXX_WRAP_PATH variable.")
  endif()
endif()
message(STATUS "Julia CxxWrap path: " "${JL_CXX_WRAP_PATH}")
list(APPEND CMAKE_PREFIX_PATH "${JL_CXX_WRAP_PATH}")
find_package(JlCxx)

get_target_property(JlCxx_location JlCxx::cxxwrap_julia LOCATION)
get_filename_component(JlCxx_location ${JlCxx_location} DIRECTORY)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib;${JlCxx_location}")

message(STATUS "Found JlCxx at ${JlCxx_location}")

add_library(testlib SHARED testlib.cpp)

target_link_libraries(testlib JlCxx::cxxwrap_julia)

install(TARGETS
  testlib
LIBRARY DESTINATION lib
ARCHIVE DESTINATION lib
RUNTIME DESTINATION lib)
