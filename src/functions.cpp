#include "jlcxx/functions.hpp"
#include "jlcxx/module.hpp"

// This header provides helper functions to call Julia functions from C++

namespace jlcxx
{

JuliaFunction::JuliaFunction(const std::string& name, const std::string& module_name)
{
  jl_module_t* mod = nullptr;
  jl_module_t* current_mod = nullptr;
  if(registry().has_current_module())
  {
    current_mod = registry().current_module().julia_module();
  }
  if(!module_name.empty())
  {
    mod = (jl_module_t*)jl_get_global(jl_main_module, jl_symbol(module_name.c_str()));
    if(mod == nullptr && current_mod != nullptr)
    {
      mod = (jl_module_t *)jl_get_global(current_mod, jl_symbol(module_name.c_str()));
    }
    if(mod == nullptr)
    {
      throw std::runtime_error("Could not find module " + module_name + " when looking up function " + name);
    }
  }
  if(mod == nullptr)
  {
    mod = current_mod == nullptr ? jl_main_module : current_mod;
  }

  m_function = jl_get_function(mod, name.c_str());
  if(m_function == nullptr)
  {
    throw std::runtime_error("Could not find function " + name);
  }
}

JuliaFunction::JuliaFunction(jl_function_t* fpointer)
{
  if(fpointer == nullptr)
  {
    throw std::runtime_error("Storing a null function pointer in a JuliaFunction is not allowed");
  }
  m_function = fpointer;
}

}
