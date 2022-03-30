#include <jlcxx/jlcxx.hpp>
#include <jlcxx/functions.hpp>

template<typename T>
void test_type(std::string jl_type_name)
{
    if (jlcxx::julia_type_name(jlcxx::julia_type<T>()) != jl_type_name)
    {
        throw std::runtime_error(jl_type_name + " type test failed");
    }
}

int main()
{
  jlcxx::cxxwrap_init();

  test_type<char>("CxxChar");
  test_type<char16_t>("CxxChar16");
  test_type<char32_t>("CxxChar32");
  test_type<bool>("CxxBool");

  jl_atexit_hook(0);
  return 0;
}
