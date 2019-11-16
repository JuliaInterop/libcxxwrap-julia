#include "jlcxx/array.hpp"
#include "jlcxx/jlcxx.hpp"
#include "jlcxx/jlcxx_config.hpp"

extern "C"
{

using namespace jlcxx;

/// Initialize the module
JLCXX_API void initialize(jl_value_t* julia_module, jl_value_t* cppfunctioninfo_type, void* gc_protect_f, void* gc_unprotect_f)
{
  if(g_cxxwrap_module != nullptr)
  {
    if((jl_module_t*)julia_module != g_cxxwrap_module)
    {
      throw std::runtime_error("Two different CxxWrap modules are loaded, aborting.");
    }
    return;
  }

  g_cxxwrap_module = (jl_module_t*)julia_module;
  g_cppfunctioninfo_type = (jl_datatype_t*)cppfunctioninfo_type;
  g_protect_from_gc = reinterpret_cast<protect_f_t>(gc_protect_f);
  g_unprotect_from_gc = reinterpret_cast<protect_f_t>(gc_unprotect_f);

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
      f.return_type();
    });
    jlcxx::registry().reset_current_module();
  }
  catch (const std::runtime_error& e)
  {
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
JLCXX_API void bind_module_constants(jl_value_t* module_any)
{
  jl_module_t* mod = (jl_module_t*)module_any;
  registry().get_module(mod).bind_constants(mod);
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

    boxed_f = jlcxx::box<cxxint_t>(f.pointer_index());
    boxed_thunk = jlcxx::box<cxxint_t>(f.thunk_index());

    function_array.push_back(jl_new_struct(g_cppfunctioninfo_type,
      f.name(),
      arg_types_array.wrapped(),
      f.return_type(),
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

JLCXX_API const char* version_string()
{
  return JLCXX_VERSION_STRING;
}

}
