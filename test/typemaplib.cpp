#include "typemaptest.h"
#include <iostream>
#include <unordered_map>

TestType::TestType()
{
  std::cout << "constructing TestType" << std::endl;
}

EXPORTAPI StoredType& get_stored_type_hash(std::type_index idx)
{
  static std::unordered_map<std::type_index, StoredType> m_map;
  if(m_map.count(idx) == 0)
  {
    m_map[idx] = StoredType();
  }
  return m_map[idx];
}

extern "C"
{

EXPORTAPI void init_types()
{
  std::cout << "Initializing types..." << std::endl;
  get_stored_type<int>().m_name = "int";
  get_stored_type<TestType>().m_name = "TestType";

  get_stored_type_hash(typeid(int)).m_name = "int";
  get_stored_type_hash(typeid(TestType)).m_name = "TestType";
}

}

