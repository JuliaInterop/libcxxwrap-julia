#include "jlcxx/array.hpp"
#include "jlcxx/jlcxx.hpp"
#include "jlcxx/jlcxx_config.hpp"

#include "julia_gcext.h"

namespace jlcxx
{

template<typename SourceT>
struct BuildEquivalenceInner
{
  template<typename T>
  void operator()()
  {
    if(std::is_same<SourceT,T>::value)
    {
      m_fundamental_types_matched.push_back(jl_cstr_to_string(fundamental_int_type_name<SourceT>().c_str()));
      m_equivalent_types.push_back(jl_cstr_to_string(fixed_int_type_name<T>().c_str()));
    }
  }

  ArrayRef<jl_value_t*> m_fundamental_types_matched;
  ArrayRef<jl_value_t*> m_equivalent_types;
};

struct BuildEquivalence
{
  template<typename T>
  void operator()()
  {
    for_each_type<fixed_int_types>(BuildEquivalenceInner<T>{m_fundamental_types_matched, m_equivalent_types});
  }

  ArrayRef<jl_value_t*> m_fundamental_types_matched;
  ArrayRef<jl_value_t*> m_equivalent_types;
};

struct GetFundamentalTypes
{
  template<typename T>
  void operator()()
  {
    m_types.push_back(jl_cstr_to_string(fundamental_int_type_name<T>().c_str()));
    m_type_sizes.push_back(jl_box_int32(static_cast<int>(sizeof(T))));
  }
  ArrayRef<jl_value_t*> m_types;
  ArrayRef<jl_value_t*> m_type_sizes;
};

}

extern "C"
{

using namespace jlcxx;

/// Initialize the module
JLCXX_API void initialize_cxxwrap(jl_value_t* julia_module, jl_value_t* cppfunctioninfo_type)
{
  if(g_cxxwrap_module != nullptr)
  {
    if((jl_module_t*)julia_module != g_cxxwrap_module)
    {
      throw std::runtime_error("Two different CxxWrap modules are loaded, aborting.");
    }
    return;
  }

  jl_gc_set_cb_root_scanner(cxx_root_scanner, 1);

  g_cxxwrap_module = (jl_module_t*)julia_module;
  g_cppfunctioninfo_type = (jl_datatype_t*)cppfunctioninfo_type;

  register_core_types();
  register_core_cxxwrap_types();
}

JLCXX_API void register_julia_module(jl_module_t* jlmod, void (*regfunc)(jlcxx::Module&))
{
  try
  {
    jlcxx::Module& mod = jlcxx::registry().create_module(jlmod);
    regfunc(mod);
    mod.for_each_function([] (FunctionWrapperBase& f) {
      // Make sure any pointers in the types are also resolved at module init.
      f.argument_types();
    });
    jlcxx::registry().reset_current_module();
  }
  catch (const std::runtime_error& e)
  {
    std::cerr << "C++ exception while wrapping module " << module_name(jlmod) << ": " << e.what()  << std::endl;
    jl_error(e.what());
  }
}

JLCXX_API bool has_cxx_module(jl_module_t* jlmod)
{
  return jlcxx::registry().has_module(jlmod);
}

JLCXX_API jl_module_t* get_cxxwrap_module()
{
  return g_cxxwrap_module;
}

/// Bind jl_datatype_t structures to corresponding Julia symbols in the given module
JLCXX_API void bind_module_constants(jl_value_t* module_any, jl_value_t* symbols, jl_value_t* values)
{
  jl_module_t* mod = (jl_module_t*)module_any;
  registry().get_module(mod).bind_constants(ArrayRef<jl_value_t*>((jl_array_t*)symbols), ArrayRef<jl_value_t*>((jl_array_t*)values));
}

void fill_types_vec(Array<jl_datatype_t*>& types_array, const std::vector<jl_datatype_t*>& types_vec)
{
  for(const auto& t : types_vec)
  {
    types_array.push_back(t);
  }
}

/// Get the functions defined in the modules. Any classes used by these functions must be defined on the Julia side first
JLCXX_API jl_array_t* get_module_functions(jl_module_t* jlmod)
{
  Array<jl_value_t*> function_array(g_cppfunctioninfo_type);
  JL_GC_PUSH1(function_array.gc_pointer());

  const jlcxx::Module& module = registry().get_module(jlmod);
  module.for_each_function([&](FunctionWrapperBase& f)
  {
    Array<jl_datatype_t*> arg_types_array;
    jl_value_t* boxed_f = nullptr;
    jl_value_t* boxed_thunk = nullptr;
    JL_GC_PUSH3(arg_types_array.gc_pointer(), &boxed_f, &boxed_thunk);

    fill_types_vec(arg_types_array, f.argument_types());

    boxed_f = jlcxx::box<void*>(f.pointer());
    boxed_thunk = jlcxx::box<void*>(f.thunk());

    auto returntypes = f.return_type();

    jl_datatype_t* ccall_return_type = returntypes.first;
    jl_datatype_t* julia_return_type = returntypes.second;
    if(ccall_return_type == nullptr)
    {
      ccall_return_type = julia_type<void>();
      julia_return_type = ccall_return_type;
    }

    function_array.push_back(jl_new_struct(g_cppfunctioninfo_type,
      f.name(),
      arg_types_array.wrapped(),
      ccall_return_type,
      julia_return_type,
      boxed_f,
      boxed_thunk,
      f.override_module()
    ));

    JL_GC_POP();
  });
  JL_GC_POP();
  return function_array.wrapped();
}

jl_array_t* convert_type_vector(const std::vector<jl_datatype_t*> types_vec)
{
  Array<jl_datatype_t*> datatypes;
  JL_GC_PUSH1(datatypes.gc_pointer());
  for(jl_datatype_t* dt : types_vec)
  {
    datatypes.push_back(dt);
  }
  JL_GC_POP();
  return datatypes.wrapped();
}

JLCXX_API jl_array_t* get_box_types(jl_module_t* jlmod)
{
  return convert_type_vector(registry().get_module(jlmod).box_types());
}

JLCXX_API const char* cxxwrap_version_string()
{
  return JLCXX_VERSION_STRING;
}

JLCXX_API void gcprotect(jl_value_t* v)
{
  protect_from_gc(v);
}

JLCXX_API void gcunprotect(jl_value_t* v)
{
  unprotect_from_gc(v);
}

JLCXX_API void get_integer_types(jl_value_t* all_fundamental_types, jl_value_t* type_sizes, jl_value_t* fundamental_types_matched, jl_value_t* equivalent_types)
{
  for_each_type<fundamental_int_types>(GetFundamentalTypes{ArrayRef<jl_value_t*>((jl_array_t*)all_fundamental_types), ArrayRef<jl_value_t*>((jl_array_t*)type_sizes)});
  for_each_type<fundamental_int_types>(BuildEquivalence{ArrayRef<jl_value_t*>((jl_array_t*)fundamental_types_matched), ArrayRef<jl_value_t*>((jl_array_t*)equivalent_types)});
}

}
