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
  apply_vector(m_instance->vector);
  apply_valarray(m_instance->valarray);
  apply_deque_iterator(m_instance->deque_iterator);
  apply_deque(m_instance->deque);
  apply_queue(m_instance->queue);
  apply_priority_queue(m_instance->priority_queue);
  apply_stack(m_instance->stack);
  apply_set_iterator(m_instance->set_iterator);
  apply_set(m_instance->set);
  apply_multiset_iterator(m_instance->multiset_iterator);
  apply_multiset(m_instance->multiset);
  apply_unordered_set_iterator(m_instance->unordered_set_iterator);
  apply_unordered_set(m_instance->unordered_set);
  apply_unordered_multiset_iterator(m_instance->unordered_multiset_iterator);
  apply_unordered_multiset(m_instance->unordered_multiset);
  apply_list_iterator(m_instance->list_iterator);
  apply_list(m_instance->list);
  apply_forward_list_iterator(m_instance->forward_list_iterator);
  apply_forward_list(m_instance->forward_list);
  apply_shared_ptr();
  apply_weak_ptr();
  apply_unique_ptr();
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
  valarray(stl.add_type<Parametric<TypeVar<1>>>("StdValArray", julia_type("AbstractVector"))),
  deque_iterator(stl.add_type<Parametric<TypeVar<1>>>("StdDequeIterator")),
  deque(stl.add_type<Parametric<TypeVar<1>>>("StdDeque", julia_type("AbstractVector"))),
  queue(stl.add_type<Parametric<TypeVar<1>>>("StdQueue")),
  priority_queue(stl.add_type<Parametric<TypeVar<1>>>("StdPriorityQueue")),
  stack(stl.add_type<Parametric<TypeVar<1>>>("StdStack")),
  set_iterator(stl.add_type<Parametric<TypeVar<1>>>("StdSetIterator")),
  set(stl.add_type<Parametric<TypeVar<1>>>("StdSet", julia_type("AbstractSet"))),
  multiset_iterator(stl.add_type<Parametric<TypeVar<1>>>("StdMultisetIterator")),
  multiset(stl.add_type<Parametric<TypeVar<1>>>("StdMultiset", julia_type("AbstractSet"))),
  unordered_set_iterator(stl.add_type<Parametric<TypeVar<1>>>("StdUnorderedSetIterator")),
  unordered_set(stl.add_type<Parametric<TypeVar<1>>>("StdUnorderedSet", julia_type("AbstractSet"))),
  unordered_multiset_iterator(stl.add_type<Parametric<TypeVar<1>>>("StdUnorderedMultisetIterator")),
  unordered_multiset(stl.add_type<Parametric<TypeVar<1>>>("StdUnorderedMultiset", julia_type("AbstractSet"))),
  list_iterator(stl.add_type<Parametric<TypeVar<1>>>("StdListIterator")),
  list(stl.add_type<Parametric<TypeVar<1>>>("StdList")),
  forward_list_iterator(stl.add_type<Parametric<TypeVar<1>>>("StdForwardListIterator")),
  forward_list(stl.add_type<Parametric<TypeVar<1>>>("StdForwardList"))
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
  wrapper.module().method("cxxgetindex", [] (const string_t& s, cxxint_t i) { return s[i-1]; });
}

}

JLCXX_MODULE define_cxxwrap_stl_module(jlcxx::Module& stl)
{
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

  jlcxx::stl::StlWrappers::instantiate(stl);
}

}

