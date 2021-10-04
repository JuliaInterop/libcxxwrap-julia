#include <jlcxx/jlcxx.hpp>
#include <jlcxx/functions.hpp>

int main()
{
  jlcxx::cxxwrap_init();
  if (jlcxx::julia_type_name(jlcxx::julia_type<char>()) != "CxxChar")
  {
    throw std::runtime_error("char type test failed");
  }
  if(jlcxx::julia_type_name(jlcxx::julia_type<bool>()) != "CxxBool")
  {
    throw std::runtime_error("unsigned bool test failed");
  }
  jl_atexit_hook(0);
  return 0;
}
