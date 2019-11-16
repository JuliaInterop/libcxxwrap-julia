#include <julia.h>
#include <jlcxx/jlcxx.hpp>
#include <jlcxx/functions.hpp>

int main()
{
  jl_init();
  jlcxx::register_core_types();
  jl_atexit_hook(0);
  return 0;
}
