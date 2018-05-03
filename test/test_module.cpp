#include <julia.h>
#include <jlcxx/jlcxx.hpp>

namespace test_module
{

class Foo
{
public:
  Foo(int x = 0) : m_data(x)
  {
  }

  int getx() const
  {
    return m_data;
  }

private:
  int m_data;
};

}

JULIA_CPP_MODULE_BEGIN(registry)
  jlcxx::Module& mod = registry.create_module("TestModule");

  using namespace test_module;

  mod.add_type<Foo>("Foo")
    .method("getx", &Foo::getx);

JULIA_CPP_MODULE_END

extern "C"
{
extern void initialize(jl_value_t* julia_module, jl_value_t* cpp_any_type, jl_value_t* cppfunctioninfo_type);
extern void* create_registry(jl_value_t* parent_module, jl_value_t* wrapped_module);
extern void bind_module_constants(void* void_registry, jl_value_t* module_any);
}

int main()
{
  jl_init();

  jl_eval_string("include(\"cxxwrap_testmod.jl\")");
  if (jl_exception_occurred())
  {
    jl_call2(jl_get_function(jl_base_module, "showerror"), jl_stderr_obj(), jl_exception_occurred());
    jl_printf(jl_stderr_stream(), "\n");
    jl_atexit_hook(1);
    return 1;
  }

  jl_value_t* mod = jl_eval_string("TestModule = Module(:TestModule)");
  if (jlcxx::julia_type_name(jl_typeof(mod)) != "Module")
  {
    std::cout << "TestModule creation failed" << std::endl;
    return 1;
  }

  initialize(jl_eval_string("CxxWrap"), jl_eval_string("CxxWrap.CppAny"), jl_eval_string("CxxWrap.CppFunctionInfo"));

  JL_GC_PUSH1(&mod);

  void* reg = create_registry((jl_value_t*)jl_main_module, (jl_value_t*)mod);
  register_julia_modules(reg);
  bind_module_constants(reg, mod);

  jl_value_t* dt = jl_eval_string("TestModule.Foo");
  if(jlcxx::julia_type_name(dt) != "Foo")
  {
    std::cout << "unexpected type name: " << jlcxx::julia_type_name(dt) << std::endl;
    return 1;
  }

  JL_GC_POP();
  
  jl_atexit_hook(0);
  return 0;
}
