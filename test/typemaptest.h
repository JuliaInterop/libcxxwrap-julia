#include <string>
#include <typeindex>
#include <typeinfo>

#ifdef _WIN32
  #define EXPORTAPI __declspec(dllexport)
#else
  #define EXPORTAPI __attribute__ ((visibility("default")))
#endif

struct TestType
{
  TestType();
};

struct StoredType
{
  std::string m_name = "";
};

template<typename CppT>
StoredType& get_stored_type()
{
  static StoredType stored;
  return stored;
}

EXPORTAPI StoredType& get_stored_type_hash(std::type_index idx);
