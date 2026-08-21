#include "typemaptest.h"
#include <iostream>

extern "C"
{

EXPORTAPI int show_types()
{
  int result = 0;

  std::cout << "Type int stored as: " << get_stored_type<int>().m_name << std::endl;
  std::cout << "Type TestType stored as: " << get_stored_type<TestType>().m_name << std::endl;

  if(get_stored_type<int>().m_name != "int" || get_stored_type<TestType>().m_name != "TestType")
  {
    std::cout << "static variable is not shared on this platform" << std::endl;
#ifndef _WIN32 // Windows uses the hashed type map
    result = 1;
#endif
  }

  std::cout << "Type int type_index stored as: " << get_stored_type_hash(typeid(int)).m_name << std::endl;
  std::cout << "Type TestType type_index stored as: " << get_stored_type_hash(typeid(TestType)).m_name << std::endl;

  if(get_stored_type_hash(typeid(int)).m_name != "int" || get_stored_type_hash(typeid(TestType)).m_name != "TestType")
  {
    std::cout << "std::type_index is not unique on this platform" << std::endl;
    result = 1;
  }

  return result;
}

}
