﻿#include "jlcxx/array.hpp"
#include "jlcxx/jlcxx.hpp"
#include "jlcxx/functions.hpp"
#include "jlcxx/jlcxx_config.hpp"

#include <julia.h>
#if JULIA_VERSION_MAJOR == 0 && JULIA_VERSION_MINOR > 4 || JULIA_VERSION_MAJOR > 0
#include <julia_threads.h>
#endif

namespace jlcxx
{

jl_module_t* g_cxxwrap_module;
jl_datatype_t* g_cppfunctioninfo_type;

JLCXX_API jl_array_t* gc_protected()
{
  static jl_array_t* m_arr = nullptr;
  if (m_arr == nullptr)
  {
#if JULIA_VERSION_MAJOR == 0 && JULIA_VERSION_MINOR > 4 || JULIA_VERSION_MAJOR > 0
    jl_value_t* array_type = apply_array_type(jl_any_type, 1);
    m_arr = jl_alloc_array_1d(array_type, 0);
#else
    m_arr = jl_alloc_cell_1d(0);
#endif
    jl_set_const(g_cxxwrap_module, jl_symbol("_gc_protected"), (jl_value_t*)m_arr);
  }
  return m_arr;
}

JLCXX_API std::stack<std::size_t>& gc_free_stack()
{
  static std::stack<std::size_t> m_stack;
  return m_stack;
}

JLCXX_API std::map<jl_value_t*, std::pair<std::size_t,std::size_t>>& gc_index_map()
{
  static std::map<jl_value_t*, std::pair<std::size_t,std::size_t>> m_map;
  return m_map;
}

Module::Module(jl_module_t* jmod) :
  m_jl_mod(jmod),
  m_pointer_array((jl_array_t*)jl_get_global(jmod, jl_symbol("__cxxwrap_pointers")))
{
}

int_t Module::store_pointer(void *ptr)
{
  assert(ptr != nullptr);
  m_pointer_array.push_back(ptr);
  return m_pointer_array.size();
}

Module &ModuleRegistry::create_module(jl_module_t* jmod)
{
  if(jmod == nullptr)
    throw std::runtime_error("Can't create module from null Julia module");
  if(m_modules.count(jmod))
    throw std::runtime_error("Error registering module: " + module_name(jmod) + " was already registered");

  m_current_module = new Module(jmod);
  m_modules[jmod].reset(m_current_module);
  return *m_current_module;
}

Module& ModuleRegistry::current_module()
{
  assert(m_current_module != nullptr);
  return *m_current_module;
}

void FunctionWrapperBase::set_pointer_indices()
{
  m_pointer_index = m_module->store_pointer(pointer());
  void* thk = thunk();
  if(thk != nullptr)
  {
    m_thunk_index = m_module->store_pointer(thunk());
  }
}

JLCXX_API ModuleRegistry& registry()
{
  static ModuleRegistry m_registry;
  return m_registry;
}

JLCXX_API jl_value_t* julia_type(const std::string& name, const std::string& module_name)
{
  const auto mods = {module_name.empty() ? nullptr : (jl_module_t*)jl_get_global(jl_current_module, jl_symbol(module_name.c_str())), jl_base_module, g_cxxwrap_module, jl_current_module, jl_current_module->parent};
  std::string found_type;
  for(jl_module_t* mod : mods)
  {
    if(mod == nullptr)
    {
      continue;
    }

    jl_value_t* gval = jl_get_global(mod, jl_symbol(name.c_str()));
#if JULIA_VERSION_MAJOR == 0 && JULIA_VERSION_MINOR < 6
    if(gval != nullptr && (jl_is_datatype(gval) || jl_is_typector(gval)))
#else
    if(gval != nullptr && (jl_is_datatype(gval) || jl_is_unionall(gval)))
#endif
    {
      return gval;
    }
    if(gval != nullptr)
    {
      found_type = julia_type_name(jl_typeof(gval));
    }
  }
  std::string errmsg = "Symbol for type " + name + " was not found. A Value of type " + found_type + " was found instead. Searched modules:";
  for(jl_module_t* mod : mods)
  {
    if(mod != nullptr)
    {
      errmsg +=  " " + symbol_name(mod->name);
    }
  }
  throw std::runtime_error(errmsg);
}

InitHooks& InitHooks::instance()
{
  static InitHooks hooks;
  return hooks;
}

InitHooks::InitHooks()
{
}

void InitHooks::add_hook(const hook_t hook)
{
  m_hooks.push_back(hook);
}

void InitHooks::run_hooks()
{
  for(const hook_t& h : m_hooks)
  {
    h();
  }
}

JLCXX_API jl_value_t* apply_type(jl_value_t* tc, jl_svec_t* params)
{
#if JULIA_VERSION_MAJOR == 0 && JULIA_VERSION_MINOR < 6
  return jl_apply_type(tc, params);
#else
  return jl_apply_type(jl_is_unionall(tc) ? tc : ((jl_datatype_t*)tc)->name->wrapper, jl_svec_data(params), jl_svec_len(params));
#endif
}

jl_value_t* ConvertToJulia<std::wstring, false, false, false>::operator()(const std::wstring& str) const
{
  static const JuliaFunction wstring_to_julia("wstring_to_julia", "CxxWrap");
  return wstring_to_julia(str.c_str(), static_cast<int_t>(str.size()));
}

std::wstring ConvertToCpp<std::wstring, false, false, false>::operator()(jl_value_t* jstr) const
{
  static const JuliaFunction wstring_to_cpp("wstring_to_cpp", "CxxWrap");
  ArrayRef<wchar_t> arr((jl_array_t*)wstring_to_cpp(jstr));
  return std::wstring(arr.data(), arr.size());
}

JLCXX_API jl_datatype_t* new_datatype(jl_sym_t *name,
                            jl_module_t* module,
                            jl_datatype_t *super,
                            jl_svec_t *parameters,
                            jl_svec_t *fnames, jl_svec_t *ftypes,
                            int abstract, int mutabl,
                            int ninitialized)
{
  if(module == nullptr)
  {
    throw std::runtime_error("null module when creating type");
  }
#if JULIA_VERSION_MAJOR == 0 && JULIA_VERSION_MINOR < 7
  return jl_new_datatype(name, super, parameters, fnames, ftypes, abstract, mutabl, ninitialized);
#else
  return jl_new_datatype(name, module, super, parameters, fnames, ftypes, abstract, mutabl, ninitialized);
#endif
}

}
