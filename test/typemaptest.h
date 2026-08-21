#include <string>
#include <typeindex>
#include <typeinfo>

#ifdef _WIN32
  #define EXPORTAPI __declspec(dllexport)
  #define EXPORTVISAPI
#else
  #define EXPORTAPI __attribute__ ((visibility("default")))
  #define EXPORTVISAPI EXPORTAPI
#endif

struct EXPORTVISAPI TestType
{
  TestType();
};

struct EXPORTVISAPI StoredType
{
  std::string m_name = "";
};

// Visibility attribute is necessary to ensure function local static is shared between libraries
template<typename CppT>
EXPORTVISAPI StoredType& get_stored_type()
{
  static StoredType stored;
  return stored;
}

EXPORTAPI StoredType& get_stored_type_hash(std::type_index idx);
