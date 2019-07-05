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
  m_instance->vector.apply_combination<std::vector, stltypes>(stl::WrapVector());
  smartptr::apply_smart_combination<std::shared_ptr, stltypes>(mod);
  smartptr::apply_smart_combination<std::weak_ptr, stltypes>(mod);
  smartptr::apply_smart_combination<std::unique_ptr, stltypes>(mod);
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
  m_stl_mod(stl),
  vector(stl.add_type<Parametric<TypeVar<1>>>("StdVector", julia_type("AbstractVector")))
{
}

template<typename string_t>
void wrap_string(TypeWrapper<string_t>&& wrapper)
{
  using char_t = typename string_t::value_type;
  wrapper
    .template constructor<const char_t*>()
    .template constructor<const char_t*, std::size_t>()
    .method("c_str", [] (const string_t& s) { return s.c_str(); })
    .method("cppsize", &string_t::size)
    .method("getindex", [] (const string_t& s, cxxint_t i) { return s[i-1]; });
}

}

JLCXX_MODULE define_julia_module(jlcxx::Module& stl)
{
  jlcxx::stl::wrap_string(stl.add_type<std::string>("StdString", julia_type("CppBasicString")));
  jlcxx::stl::wrap_string(stl.add_type<std::wstring>("StdWString", julia_type("CppBasicString")));

  jlcxx::add_smart_pointer<std::shared_ptr>(stl, "SharedPtr");
  jlcxx::add_smart_pointer<std::weak_ptr>(stl, "WeakPtr");
  jlcxx::add_smart_pointer<std::unique_ptr>(stl, "UniquePtr");

  jlcxx::stl::StlWrappers::instantiate(stl);
}

}

