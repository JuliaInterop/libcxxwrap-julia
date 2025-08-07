#define JLCXX_FORCE_RANGES_OFF
#include <jlcxx/jlcxx.hpp>
#include <jlcxx/functions.hpp>
#include <jlcxx/stl.hpp>

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

template<typename T1, typename T2, typename T3, typename T4 = void>
struct ManyParams
{
};

struct WrapManyParams
{
  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& )
  {
  }
};

}

JLCXX_MODULE register_test_module(jlcxx::Module& mod)
{
  using namespace test_module;

  mod.add_type<Foo>("Foo")
    .method("getx", &Foo::getx);

  mod.add_type<std::pair<int,Foo>>("FooPair");

  mod.method("vectortest", [] (std::vector<Foo>) {});
  mod.method("pairtest", [] (std::vector<std::pair<int,Foo>>) {});

  using namespace jlcxx;

  mod.add_type<Parametric<TypeVar<1>, TypeVar<2>, TypeVar<3>>>("ManyParams")
    .apply<ManyParams<int,int,int>>(WrapManyParams());
}

void __dummy_protect(jl_value_t*) {}

int main()
{
  jlcxx::cxxwrap_init();

  jl_value_t* mod = jl_eval_string(R"(
    module TestModule
      const __cxxwrap_pointers = Ptr{Cvoid}[]
      using CxxWrap
    end
  )");
  JL_GC_PUSH1(&mod);

  if (jlcxx::julia_type_name(jl_typeof(mod)) != "Module")
  {
    std::cout << "TestModule creation failed" << std::endl;
    return 1;
  }

  register_julia_module((jl_module_t*)mod, register_test_module);
  jl_call1(jl_get_function(jlcxx::get_cxxwrap_module(), "wraptypes"), mod);
  jl_call1(jl_get_function(jlcxx::get_cxxwrap_module(), "wrapfunctions"), mod);
  if (jl_exception_occurred())
  {
    jl_call2(jl_get_function(jl_base_module, "showerror"), jl_stderr_obj(), jl_exception_occurred());
    jl_printf(jl_stderr_stream(), "\n");
  }

  jl_value_t* dt = jl_eval_string("TestModule.Foo");
  if(jlcxx::julia_type_name(dt) != "Foo")
  {
    std::cout << "unexpected type name: " << jlcxx::julia_type_name(dt) << std::endl;
    return 1;
  }

  //jl_value_t* foo = jl_eval_string("a = TestModule.Foo(); println(a);");
  jl_eval_string("display(methods(Foo))");
  //std::cout << "foo: " << foo << std::endl;

  JL_GC_POP();
  
  jl_atexit_hook(0);
  return 0;
}
