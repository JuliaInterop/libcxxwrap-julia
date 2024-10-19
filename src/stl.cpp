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

using StoredWrappersT = std::map<std::string, TypeWrapper1>;

StoredWrappersT& stl_wrappers()
{
  static StoredWrappersT wrappers;
  return wrappers;
}

void set_wrapper(Module& stl, std::string name, jl_value_t* supertype)
{
  auto result = stl_wrappers().insert(std::make_pair(name, stl.add_type<Parametric<TypeVar<1>>>(name, supertype)));
  if(!result.second)
  {
    throw std::runtime_error("Type " + name + " was already mapped");
  }
}

TypeWrapper1& get_wrapper(std::string name)
{
  auto result = stl_wrappers().find(name);
  if(result == stl_wrappers().end())
  {
    throw std::runtime_error("Type " + name + " was not added");
  }
  return result->second;
}

bool has_wrapper(std::string name)
{
  return stl_wrappers().count(name) != 0;
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
  wrapper.module().method("cxxgetindex", [] (const string_t& s, cxxint_t i) { return s[i-1]; });
}

jl_module_t* g_stl_module = nullptr;

JLCXX_API jl_module_t* stl_module()
{
  assert(g_stl_module != nullptr);
  return g_stl_module;
}

}

JLCXX_MODULE define_cxxwrap_stl_module(jlcxx::Module& stl)
{
  stl::g_stl_module = stl.julia_module();
#ifdef JLCXX_HAS_RANGES
  stl.set_const("HAS_RANGES", 1);
#endif
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

  jlcxx::stl::apply_vector();
  jlcxx::stl::apply_valarray();
  jlcxx::stl::apply_deque();
  jlcxx::stl::apply_queue();
  jlcxx::stl::apply_priority_queue();
  jlcxx::stl::apply_stack();
  jlcxx::stl::apply_set();
  jlcxx::stl::apply_multiset();
  jlcxx::stl::apply_unordered_set();
  jlcxx::stl::apply_unordered_multiset();
  jlcxx::stl::apply_list();
  jlcxx::stl::apply_forward_list();

  jlcxx::stl::apply_shared_ptr();
  jlcxx::stl::apply_weak_ptr();
  jlcxx::stl::apply_unique_ptr();
}

}

