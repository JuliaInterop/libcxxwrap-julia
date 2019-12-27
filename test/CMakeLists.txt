include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(test_module test_module.cpp)
target_link_libraries(test_module ${JLCXX_TARGET} ${Julia_LIBRARY})
add_test(NAME test_module COMMAND test_module)

add_executable(test_type_init test_type_init.cpp)
target_link_libraries(test_type_init ${JLCXX_TARGET} ${Julia_LIBRARY})
add_test(NAME test_type_init COMMAND test_type_init)

add_executable(test_cxxwrap test_cxxwrap.cpp)
target_link_libraries(test_cxxwrap ${JLCXX_TARGET} ${Julia_LIBRARY})
add_test(NAME test_cxxwrap COMMAND test_cxxwrap)

if(WIN32)
  set_property(TEST test_module test_type_init test_cxxwrap PROPERTY
    ENVIRONMENT
      "PATH=${JULIA_HOME}\;${CMAKE_BINARY_DIR}"
      "JULIA_HOME=${JULIA_HOME}"
  )
else()
  set_property(TEST test_module test_type_init test_cxxwrap PROPERTY
    ENVIRONMENT
      "JULIA_HOME=${JULIA_HOME}"
  )
endif()