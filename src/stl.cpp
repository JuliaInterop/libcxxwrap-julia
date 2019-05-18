#include <string>
#include <vector>

#include "jlcxx/jlcxx.hpp"
#include "jlcxx/stl.hpp"

namespace jlcxx
{

namespace stl
{

std::unique_ptr<StlWrappers> StlWrappers::m_instance = std::unique_ptr<StlWrappers>();

void StlWrappers::instantiate(Module& mod)
{
  m_instance.reset(new StlWrappers(mod));
}

StlWrappers& StlWrappers::instance()
{
  if(m_instance == nullptr)
  {
    throw std::runtime_error("StlWrapper was not instantiated");
  }
  return *m_instance;
}

StlWrappers& wrappers()
{
  return StlWrappers::instance();
}

StlWrappers::StlWrappers(Module& stl) :
  vector(stl.add_type<Parametric<TypeVar<1>>>("StdVector", julia_type("AbstractVector")))
{
  vector.apply_combination<std::vector, stltypes>(stl::WrapVector());
}

}

JLCXX_MODULE define_julia_module(jlcxx::Module& stl)
{
  stl.add_type<std::string>("StdString")
    .constructor<const char*>()
    .method("c_str", [] (const std::string& s) { return s.c_str(); });
  
  stl.add_type<std::wstring>("StdWString")
    .constructor<const wchar_t*>()
    .method("c_str", [] (const std::wstring& s) { return s.c_str(); });

  jlcxx::stl::StlWrappers::instantiate(stl);
}

}

