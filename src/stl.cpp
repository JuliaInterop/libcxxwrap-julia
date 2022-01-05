#include <string>
#include <thread>
#include <vector>

#include "jlcxx/jlcxx.hpp"
#include "jlcxx/functions.hpp"
#include "jlcxx/stl.hpp"

namespace jlcxx
{

namespace stl
{

JLCXX_API std::unique_ptr<StlWrappers> StlWrappers::m_instance = std::unique_ptr<StlWrappers>();

JLCXX_API void StlWrappers::instantiate(Module& mod)
{
  m_instance.reset(new StlWrappers(mod));
  m_instance->vector.apply_combination<std::vector, stltypes>(stl::WrapVector());
  m_instance->valarray.apply_combination<std::valarray, stltypes>(stl::WrapValArray());
  smartptr::apply_smart_combination<std::shared_ptr, stltypes>(mod);
  smartptr::apply_smart_combination<std::weak_ptr, stltypes>(mod);
  smartptr::apply_smart_combination<std::unique_ptr, stltypes>(mod);
}

JLCXX_API StlWrappers& StlWrappers::instance()
{
  if(m_instance == nullptr)
  {
    throw std::runtime_error("StlWrapper was not instantiated");
  }
  return *m_instance;
}

JLCXX_API StlWrappers& wrappers()
{
  return StlWrappers::instance();
}

JLCXX_API StlWrappers::StlWrappers(Module& stl) :
  m_stl_mod(stl),
  vector(stl.add_type<Parametric<TypeVar<1>>>("StdVector", julia_type("AbstractVector"))),
  valarray(stl.add_type<Parametric<TypeVar<1>>>("StdValArray", julia_type("AbstractVector")))
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
    .method("cppsize", [] (const string_t& s) { return s.size(); });
  wrapper.module().set_override_module(jl_base_module);
  wrapper.module().method("getindex", [] (const string_t& s, cxxint_t i) { return s[i-1]; });
  wrapper.module().unset_override_module();
}

}

JLCXX_MODULE define_cxxwrap_stl_module(jlcxx::Module& stl)
{
  jlcxx::stl::wrap_string(stl.add_type<std::string>("StdString", julia_type("CppBasicString")));
  jlcxx::stl::wrap_string(stl.add_type<std::wstring>("StdWString", julia_type("CppBasicString")));

  stl.add_type<std::thread::id>("StdThreadId");
  stl.set_override_module(jl_base_module);
  stl.method("==", [] (const std::thread::id& a, const std::thread::id& b) { return a == b; });
  stl.unset_override_module();

#ifndef __FreeBSD__
  // This is unsigned long on linux
  if(!has_julia_type<std::thread::native_handle_type>())
  {
    stl.add_bits<std::thread::native_handle_type>("StdThreadNativeHandleType");
  }
#endif

  stl.add_type<std::thread>("StdThread")
    .constructor<void(*)()>()
    .method("joinable", &std::thread::joinable)
    .method("get_id", &std::thread::get_id)
#ifndef __FreeBSD__
    .method("native_handle", &std::thread::native_handle)
#endif
    .method("join", &std::thread::join)
    .method("detach", &std::thread::detach)
    .method("swap", &std::thread::swap);

  stl.method("hardware_concurrency", [] () { return std::thread::hardware_concurrency(); });
  
  jlcxx::add_smart_pointer<std::shared_ptr>(stl, "SharedPtr");
  jlcxx::add_smart_pointer<std::weak_ptr>(stl, "WeakPtr");
  jlcxx::add_smart_pointer<std::unique_ptr>(stl, "UniquePtr");

  jlcxx::stl::StlWrappers::instantiate(stl);
}

}

