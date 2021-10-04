#include <jlcxx/jlcxx.hpp>
#include <jlcxx/functions.hpp>

int main()
{
  jlcxx::cxxwrap_init();
  jl_eval_string("import Pkg; Pkg.test(\"CxxWrap\")");
  if(jl_exception_occurred())
  {
    jl_call2(jl_get_function(jl_base_module, "showerror"), jl_stderr_obj(), jl_exception_occurred());
    jl_printf(jl_stderr_stream(), "\n");
    throw std::runtime_error("error running CxxWrap tests");
  }
  jl_atexit_hook(0);
  return 0;
}
