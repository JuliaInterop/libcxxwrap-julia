include_directories(${CMAKE_SOURCE_DIR}/include)

configure_file(cxxwrap_testmod.jl ${CMAKE_CURRENT_BINARY_DIR}/cxxwrap_testmod.jl)

add_executable(test_module test_module.cpp)
target_link_libraries(test_module ${JLCXX_TARGET} ${Julia_LIBRARY})

add_test(NAME test_module COMMAND test_module)
if(WIN32)
  set_property(TEST test_module PROPERTY
    ENVIRONMENT
      "PATH=${JULIA_HOME}\;${CMAKE_BINARY_DIR}/$<CONFIGURATION>"
      "JULIA_HOME=${JULIA_HOME}"
  )
else()
  set_property(TEST test_module PROPERTY
    ENVIRONMENT
      "JULIA_HOME=${JULIA_HOME}"
  )
endif()
