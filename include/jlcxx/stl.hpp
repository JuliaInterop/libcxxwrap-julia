#ifndef JLCXX_STL_HPP
#define JLCXX_STL_HPP

#include <type_traits>
#include <valarray>
#include <vector>
#include <deque>
#include <queue>
#include <stack>
#include <set>
#include <unordered_set>
#include <list>
#include <forward_list>

#include "module.hpp"
#include "smart_pointers.hpp"
#include "type_conversion.hpp"

#ifdef JLCXX_HAS_RANGES
#include <ranges>
#endif

namespace jlcxx
{

namespace detail
{

struct UnusedT {};

/// Replace T1 by UnusedT if T1 == T2, return T1 otherwise
template<typename T1, typename T2>
struct SkipIfSameAs
{
  using type = T1;
};

template<typename T>
struct SkipIfSameAs<T,T>
{
  using type = UnusedT;
};

template<typename T1, typename T2> using skip_if_same = typename SkipIfSameAs<T1,T2>::type;

}

namespace stl
{

JLCXX_API jl_module_t* stl_module();

void set_wrapper(Module& stl, std::string name, jl_value_t* supertype);
TypeWrapper1& get_wrapper(std::string name);
bool has_wrapper(std::string name);

// Separate per-container functions to split up the compilation over multiple C++ files
void apply_vector();
void apply_valarray();
void apply_deque();
void apply_queue();
void apply_priority_queue();
void apply_stack();
void apply_set();
void apply_multiset();
void apply_unordered_set();
void apply_unordered_multiset();
void apply_list();
void apply_forward_list();
void apply_shared_ptr();
void apply_weak_ptr();
void apply_unique_ptr();

using stltypes = remove_duplicates<combine_parameterlists<combine_parameterlists<ParameterList
<
  bool,
  double,
  float,
  char,
  wchar_t,
  void*,
  std::string,
  std::wstring,
  jl_value_t*
>, fundamental_int_types>, fixed_int_types>>;

template<typename TypeWrapperT>
void wrap_range_based_fill([[maybe_unused]] TypeWrapperT& wrapped)
{
#ifdef JLCXX_HAS_RANGES
  using WrappedT = typename TypeWrapperT::type;
  using T = typename WrappedT::value_type;
  wrapped.module().set_override_module(stl_module());
  wrapped.method("StdFill", [] (WrappedT& v, const T& val) { std::ranges::fill(v, val); });
  wrapped.module().unset_override_module();
#endif
}

#ifdef JLCXX_HAS_RANGES

template <typename T>
constexpr bool has_less = std::is_invocable_v<std::ranges::less, T, T>;

#endif

template<typename TypeWrapperT>
void wrap_range_based_bsearch([[maybe_unused]] TypeWrapperT& wrapped)
{
#ifdef JLCXX_HAS_RANGES
  using WrappedT = typename TypeWrapperT::type;
  using T = typename WrappedT::value_type;
  if constexpr(has_less<T>)
  {
    wrapped.module().set_override_module(stl_module());
    wrapped.method("StdBinarySearch", [] (WrappedT& v, const T& val) { return std::ranges::binary_search(v, val); });
    wrapped.module().unset_override_module();
  }
#endif
}


template <typename ValueT, template<typename...> typename ContainerT>
struct IteratorWrapper
{
  using value_type = ValueT;
  using iterator_type = typename ContainerT<value_type>::iterator;

  iterator_type value;
};

template <typename T>
void validate_iterator(T it)
{
  using IteratorT = typename T::iterator_type;
  if (it.value == IteratorT())
  {
    throw std::runtime_error("Invalid iterator");
  }
}

struct WrapIterator
{
  template <typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    using WrappedT = typename TypeWrapperT::type;
    using ValueT = typename WrappedT::value_type;
    
    wrapped.method("iterator_next", [](WrappedT it) { ++(it.value); return it; });
    wrapped.method("iterator_value", [](WrappedT it) { validate_iterator(it); return *it.value; });
    wrapped.method("iterator_is_equal", [](WrappedT it1, WrappedT it2) {return it1.value == it2.value; });
  };
};

template<template<typename...> class ContainerT>
struct WrapSTLContainer;

namespace detail
{
  template<typename T>
  struct ApplyCombination;

  template<template<typename...> class ContainerT>
  struct ApplyCombination<WrapSTLContainer<ContainerT>>
  {
    template<typename... TypeLists, typename WrapperT, typename FunctorT>
    static void apply(WrapperT& wrapper, FunctorT&& ftor)
    {
      wrapper.template apply_combination<ContainerT, TypeLists...>(ftor);
    }
  };

  template<typename T>
  struct ApplyIteratorCombination;

  template<template<typename...> class ContainerT>
  struct ApplyIteratorCombination<WrapSTLContainer<ContainerT>>
  {
    template<typename ValueT>
    struct IteratorWrapperType : IteratorWrapper<ValueT, ContainerT> {};

    template<typename... TypeLists, typename WrapperT>
    static void apply(WrapperT& wrapper)
    {
      wrapper.template apply_combination<IteratorWrapperType, TypeLists...>(WrapIterator());
    }
  };

  template<typename T>
  struct GetIteratorWrapperType;

  template<template<typename...> class ContainerT, typename T, typename... Args>
  struct GetIteratorWrapperType<ContainerT<T,Args...>>
  {
    using type = typename ApplyIteratorCombination<WrapSTLContainer<ContainerT>>::template IteratorWrapperType<T>;
  };
}

template<typename T> using iterator_wrapper_type = typename detail::GetIteratorWrapperType<T>::type;

template<typename T, bool HasIterator=false>
struct STLTypeWrapperBase
{
  static std::string iterator_name() { return T::name() + "Iterator"; }
  TypeWrapper1& wrapper(std::string name, bool isiterator = false)
  {
    if(!has_wrapper(name))
    {
      Module& stl = registry().current_module();
      assert(stl.name() == "StdLib");
      jl_value_t* supertype = isiterator ? (jl_value_t*)jl_any_type : (jl_value_t*)T::supertype();
      set_wrapper(stl, name, supertype);
    }
    return get_wrapper(name);
  }

  TypeWrapper1& typewrapper()
  {
    return wrapper(T::name());
  }

  TypeWrapper1& iteratorwrapper()
  {
    return wrapper(iterator_name(), true);
  }

  template<typename... TypeLists>
  void apply_combination()
  {
    if constexpr (has_iterator)
    {
      detail::ApplyIteratorCombination<T>::template apply<TypeLists...>(iteratorwrapper());
    }
    detail::ApplyCombination<T>::template apply<TypeLists...>(typewrapper(), *static_cast<T*>(this));
  }

  template<typename AppliedT>
  void apply(Module& module)
  {
    if constexpr (has_iterator)
    {
      TypeWrapper1(module, iteratorwrapper()).apply<iterator_wrapper_type<AppliedT>>(WrapIterator());
    }
    TypeWrapper1(module, typewrapper()).apply<AppliedT>(*static_cast<T*>(this));
  }

  static constexpr bool has_iterator = HasIterator;
};

template<typename T>
struct WrapVectorImpl
{
  template<typename TypeWrapperT>
  static void wrap(TypeWrapperT&& wrapped)
  {
    using WrappedT = std::vector<T>;
    
    wrap_range_based_bsearch(wrapped);
    wrapped.module().set_override_module(stl_module());
    wrapped.method("push_back", static_cast<void (WrappedT::*)(const T&)>(&WrappedT::push_back));
    wrapped.method("cxxgetindex", [] (const WrappedT& v, cxxint_t i) -> typename WrappedT::const_reference { return v[i-1]; });
    wrapped.method("cxxgetindex", [] (WrappedT& v, cxxint_t i) -> typename WrappedT::reference { return v[i-1]; });
    wrapped.method("cxxsetindex!", [] (WrappedT& v, const T& val, cxxint_t i) { v[i-1] = val; });
    wrapped.module().unset_override_module();
  }
};

template<>
struct WrapVectorImpl<bool>
{
  template<typename TypeWrapperT>
  static void wrap(TypeWrapperT&& wrapped)
  {
    using WrappedT = std::vector<bool>;

    wrap_range_based_bsearch(wrapped);
    wrapped.module().set_override_module(stl_module());
    wrapped.method("push_back", [] (WrappedT& v, const bool val) { v.push_back(val); });
    wrapped.method("cxxgetindex", [] (const WrappedT& v, cxxint_t i) { return bool(v[i-1]); });
    wrapped.method("cxxsetindex!", [] (WrappedT& v, const bool val, cxxint_t i) { v[i-1] = val; });
    wrapped.module().unset_override_module();
  }
};

template<>
struct WrapSTLContainer<std::vector> : STLTypeWrapperBase<WrapSTLContainer<std::vector>>
{
  static std::string name() { return "StdVector"; }
  static jl_value_t* supertype() { return julia_type("AbstractVector"); }

  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    using WrappedT = typename TypeWrapperT::type;
    using T = typename WrappedT::value_type;
    wrapped.module().set_override_module(stl_module());
    wrapped.method("cppsize", &WrappedT::size);
    wrapped.method("resize", [] (WrappedT& v, const cxxint_t s) { v.resize(s); });
    wrapped.method("append", [] (WrappedT& v, jlcxx::ArrayRef<T> arr)
    {
      const std::size_t addedlen = arr.size();
      v.reserve(v.size() + addedlen);
      for(size_t i = 0; i != addedlen; ++i)
      {
        v.push_back(arr[i]);
      }
    });
    wrapped.module().unset_override_module();
    WrapVectorImpl<T>::wrap(wrapped);
  }
};

template<>
struct WrapSTLContainer<std::valarray> : STLTypeWrapperBase<WrapSTLContainer<std::valarray>>
{
  static std::string name() { return "StdValArray"; }
  static jl_value_t* supertype() { return julia_type("AbstractVector"); }

  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    using WrappedT = typename TypeWrapperT::type;
    using T = typename WrappedT::value_type;

    wrapped.template constructor<std::size_t>();
    wrapped.template constructor<const T&, std::size_t>();
    wrapped.template constructor<const T*, std::size_t>();
    wrapped.module().set_override_module(stl_module());
    wrapped.method("cppsize", &WrappedT::size);
    wrapped.method("resize", [] (WrappedT& v, const cxxint_t s) { v.resize(s); });
    wrapped.method("cxxgetindex", [] (const WrappedT& v, cxxint_t i) { return v[i-1]; });
    wrapped.method("cxxgetindex", [] (WrappedT& v, cxxint_t i) { return v[i-1]; });
    wrapped.method("cxxsetindex!", [] (WrappedT& v, const T& val, cxxint_t i) { v[i-1] = val; });
    wrapped.module().unset_override_module();
  }
};

template<>
struct WrapSTLContainer<std::deque> : STLTypeWrapperBase<WrapSTLContainer<std::deque>,true>
{
  static std::string name() { return "StdDeque"; }
  static jl_value_t* supertype() { return julia_type("AbstractVector"); }

  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    using WrappedT = typename TypeWrapperT::type;
    using T = typename WrappedT::value_type;

    wrap_range_based_bsearch(wrapped);
    wrapped.template constructor<std::size_t>();
    wrapped.module().set_override_module(stl_module());
    wrapped.method("cppsize", &WrappedT::size);
    wrapped.method("resize", [](WrappedT &v, const cxxint_t s) { v.resize(s); });
    wrapped.method("cxxgetindex", [](const WrappedT& v, cxxint_t i) { return v[i - 1]; });
    wrapped.method("cxxsetindex!", [](WrappedT& v, const T& val, cxxint_t i) { v[i - 1] = val; });
    wrapped.method("push_back!", [] (WrappedT& v, const T& val) { v.push_back(val); });
    wrapped.method("push_front!", [] (WrappedT& v, const T& val) { v.push_front(val); });
    wrapped.method("pop_back!", [] (WrappedT& v) { v.pop_back(); });
    wrapped.method("pop_front!", [] (WrappedT& v) { v.pop_front(); });
    wrapped.method("iteratorbegin", [] (WrappedT& v) { return iterator_wrapper_type<WrappedT>{v.begin()}; });
    wrapped.method("iteratorend", [] (WrappedT& v) { return iterator_wrapper_type<WrappedT>{v.end()}; });
    wrapped.module().unset_override_module();
  }
};

template<typename T>
struct WrapQueueImpl
{
  template<typename TypeWrapperT>
  static void wrap(TypeWrapperT&& wrapped)
  {
    using WrappedT = std::queue<T>;
    
    wrapped.module().set_override_module(stl_module());
    wrapped.method("cppsize", &WrappedT::size);
    wrapped.method("q_empty", [] (WrappedT& v) { return v.empty(); });
    wrapped.method("push_back!", [] (WrappedT& v, const T& val) { v.push(val); });
    wrapped.method("front", [] (WrappedT& v) { return v.front(); });
    wrapped.method("pop_front!", [] (WrappedT& v) { v.pop(); });
    wrapped.module().unset_override_module();
  }
};

template<>
struct WrapQueueImpl<bool>
{
  template<typename TypeWrapperT>
  static void wrap(TypeWrapperT&& wrapped)
  {
    using WrappedT = std::queue<bool>;

    wrapped.module().set_override_module(stl_module());
    wrapped.method("cppsize", &WrappedT::size);
    wrapped.method("q_empty", [] (WrappedT& v) { return v.empty(); });
    wrapped.method("push_back!", [] (WrappedT& v, const bool val) { v.push(val); });
    wrapped.method("front", [] (WrappedT& v) { return v.front(); });
    wrapped.method("pop_front!", [] (WrappedT& v) { v.pop(); });
    wrapped.module().unset_override_module();
  }
};

template<>
struct WrapSTLContainer<std::queue> : STLTypeWrapperBase<WrapSTLContainer<std::queue>,false>
{
  static std::string name() { return "StdQueue"; }
  static jl_datatype_t* supertype() { return jl_any_type; }

  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    using WrappedT = typename TypeWrapperT::type;
    using T = typename WrappedT::value_type;
    WrapQueueImpl<T>::wrap(wrapped);
  }
};

template<>
struct WrapSTLContainer<std::priority_queue> : STLTypeWrapperBase<WrapSTLContainer<std::priority_queue>,false>
{
  static std::string name() { return "StdPriorityQueue"; }
  static jl_datatype_t* supertype() { return jl_any_type; }

  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    using WrappedT = typename TypeWrapperT::type;
    using T = typename WrappedT::value_type;

    wrapped.template constructor<>();
    wrapped.module().set_override_module(stl_module());
    wrapped.method("cppsize", &WrappedT::size);
    wrapped.method("pq_push!", [] (WrappedT& v, const T& val) { v.push(val); });
    wrapped.method("pq_pop!", [] (WrappedT& v) { v.pop(); });
    if constexpr(std::is_same<T,bool>::value)
    {
      wrapped.method("pq_top", [] (WrappedT& v) { return bool(v.top()); });
    }
    else
    {
      wrapped.method("pq_top", [] (WrappedT& v) { return v.top(); });
    }
    wrapped.method("pq_isempty", [] (WrappedT& v) { return v.empty(); });
    wrapped.module().unset_override_module();
  }
};

template<>
struct WrapSTLContainer<std::stack> : STLTypeWrapperBase<WrapSTLContainer<std::stack>,false>
{
  static std::string name() { return "StdStack"; }
  static jl_datatype_t* supertype() { return jl_any_type; }

  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    using WrappedT = typename TypeWrapperT::type;
    using T = typename WrappedT::value_type;

    wrapped.template constructor<>();
    wrapped.module().set_override_module(stl_module());
    wrapped.method("cppsize", &WrappedT::size);
    wrapped.method("stack_isempty", [] (WrappedT& v) { return v.empty(); });
    wrapped.method("stack_push!", [] (WrappedT& v, const T& val) { v.push(val); });
    wrapped.method("stack_top", [] (WrappedT& v) { return v.top(); });
    wrapped.method("stack_pop!", [] (WrappedT& v) { v.pop(); });
    wrapped.module().unset_override_module();
  }
};

template<>
struct WrapSTLContainer<std::set> : STLTypeWrapperBase<WrapSTLContainer<std::set>,true>
{
  static std::string name() { return "StdSet"; }
  static jl_value_t* supertype() { return julia_type("AbstractSet"); }

  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    using WrappedT = typename TypeWrapperT::type;
    using T = typename WrappedT::value_type;

    wrap_range_based_bsearch(wrapped);
    wrapped.template constructor<>();
    wrapped.module().set_override_module(stl_module());
    wrapped.method("cppsize", &WrappedT::size);
    wrapped.method("set_insert!", [] (WrappedT& v, const T& val) { v.insert(val); });
    wrapped.method("set_empty!", [] (WrappedT& v) { v.clear(); });
    wrapped.method("set_isempty", [] (WrappedT& v) { return v.empty(); });
    wrapped.method("set_delete!", [] (WrappedT&v, const T& val) { v.erase(val); });
    wrapped.method("set_in", [] (WrappedT& v, const T& val) { return v.count(val) != 0; });
    wrapped.method("iteratorbegin", [] (WrappedT& v) { return iterator_wrapper_type<WrappedT>{v.begin()}; });
    wrapped.method("iteratorend", [] (WrappedT& v) { return iterator_wrapper_type<WrappedT>{v.end()}; });
#ifdef JLCXX_HAS_RANGES
    if constexpr(has_less<T>)
    {
      wrapped.method("StdUpperBound", [] (WrappedT& v, const T& val) { return iterator_wrapper_type<WrappedT>{std::ranges::upper_bound(v, val)}; });
      wrapped.method("StdLowerBound", [] (WrappedT& v, const T& val) { return iterator_wrapper_type<WrappedT>{std::ranges::lower_bound(v, val)}; });
    }
#endif
    wrapped.module().unset_override_module();
  }
};

template<>
struct WrapSTLContainer<std::unordered_set> : STLTypeWrapperBase<WrapSTLContainer<std::unordered_set>,true>
{
  static std::string name() { return "StdUnorderedSet"; }
  static jl_value_t* supertype() { return julia_type("AbstractSet"); }

  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    using WrappedT = typename TypeWrapperT::type;
    using T = typename WrappedT::value_type;

    wrapped.template constructor<>();
    wrapped.module().set_override_module(stl_module());
    wrapped.method("cppsize", &WrappedT::size);
    wrapped.method("set_insert!", [] (WrappedT& v, const T& val) { v.insert(val); });
    wrapped.method("set_empty!", [] (WrappedT& v) { v.clear(); });
    wrapped.method("set_isempty", [] (WrappedT& v) { return v.empty(); });
    wrapped.method("set_delete!", [] (WrappedT&v, const T& val) { v.erase(val); });
    wrapped.method("set_in", [] (WrappedT& v, const T& val) { return v.count(val) != 0; });
    wrapped.method("iteratorbegin", [] (WrappedT& v) { return iterator_wrapper_type<WrappedT>{v.begin()}; });
    wrapped.method("iteratorend", [] (WrappedT& v) { return iterator_wrapper_type<WrappedT>{v.end()}; });
    wrapped.module().unset_override_module();
  }
};

template<>
struct WrapSTLContainer<std::multiset> : STLTypeWrapperBase<WrapSTLContainer<std::multiset>,true>
{
  static std::string name() { return "StdMultiset"; }
  static jl_value_t* supertype() { return julia_type("AbstractSet"); }

  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    using WrappedT = typename TypeWrapperT::type;
    using T = typename WrappedT::value_type;

    wrap_range_based_bsearch(wrapped);
    wrapped.template constructor<>();
    wrapped.module().set_override_module(stl_module());
    wrapped.method("cppsize", &WrappedT::size);
    wrapped.method("multiset_insert!", [] (WrappedT& v, const T& val) { v.insert(val); });
    wrapped.method("multiset_empty!", [] (WrappedT& v) { v.clear(); });
    wrapped.method("multiset_isempty", [] (WrappedT& v) { return v.empty(); });
    wrapped.method("multiset_delete!", [] (WrappedT&v, const T& val) { v.erase(val); });
    wrapped.method("multiset_in", [] (WrappedT& v, const T& val) { return v.count(val) != 0; });
    wrapped.method("multiset_count", [] (WrappedT& v, const T& val) { return v.count(val); });
    wrapped.method("iteratorbegin", [] (WrappedT& v) { return iterator_wrapper_type<WrappedT>{v.begin()}; });
    wrapped.method("iteratorend", [] (WrappedT& v) { return iterator_wrapper_type<WrappedT>{v.end()}; });
#ifdef JLCXX_HAS_RANGES
    if constexpr(has_less<T>)
    {
      wrapped.method("StdUpperBound", [] (WrappedT& v, const T& val) { return iterator_wrapper_type<WrappedT>{std::ranges::upper_bound(v, val)}; });
      wrapped.method("StdLowerBound", [] (WrappedT& v, const T& val) { return iterator_wrapper_type<WrappedT>{std::ranges::lower_bound(v, val)}; });
    }
#endif
    wrapped.module().unset_override_module();
  }
};

template<>
struct WrapSTLContainer<std::unordered_multiset> : STLTypeWrapperBase<WrapSTLContainer<std::unordered_multiset>,true>
{
  static std::string name() { return "StdUnorderedMultiset"; }
  static jl_value_t* supertype() { return julia_type("AbstractSet"); }

  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    using WrappedT = typename TypeWrapperT::type;
    using T = typename WrappedT::value_type;

    wrapped.template constructor<>();
    wrapped.module().set_override_module(stl_module());
    wrapped.method("cppsize", &WrappedT::size);
    wrapped.method("multiset_insert!", [] (WrappedT& v, const T& val) { v.insert(val); });
    wrapped.method("multiset_empty!", [] (WrappedT& v) { v.clear(); });
    wrapped.method("multiset_isempty", [] (WrappedT& v) { return v.empty(); });
    wrapped.method("multiset_delete!", [] (WrappedT&v, const T& val) { v.erase(val); });
    wrapped.method("multiset_in", [] (WrappedT& v, const T& val) { return v.count(val) != 0; });
    wrapped.method("multiset_count", [] (WrappedT& v, const T& val) { return v.count(val); });
    wrapped.method("iteratorbegin", [] (WrappedT& v) { return iterator_wrapper_type<WrappedT>{v.begin()}; });
    wrapped.method("iteratorend", [] (WrappedT& v) { return iterator_wrapper_type<WrappedT>{v.end()}; });
    wrapped.module().unset_override_module();
  }
};

template<>
struct WrapSTLContainer<std::list> : STLTypeWrapperBase<WrapSTLContainer<std::list>,true>
{
  static std::string name() { return "StdList"; }
  static jl_value_t* supertype() { return (jl_value_t*)jl_any_type; }

  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    using WrappedT = typename TypeWrapperT::type;
    using T = typename WrappedT::value_type;

    wrap_range_based_fill(wrapped);
    wrap_range_based_bsearch(wrapped);
    wrapped.template constructor<>();
    wrapped.module().set_override_module(stl_module());\
    wrapped.method("cppsize", &WrappedT::size);
    wrapped.method("list_empty!", [] (WrappedT& v) { v.clear(); });
    wrapped.method("list_isempty", [] (WrappedT& v) { return v.empty(); });
    wrapped.method("list_front", [] (WrappedT& v) { return v.front(); });
    wrapped.method("list_back", [] (WrappedT& v) { return v.back(); });
    wrapped.method("list_push_back!", [] (WrappedT& v, const T& val) { v.push_back(val); });
    wrapped.method("list_push_front!", [] (WrappedT& v, const T& val) { v.push_front(val); });
    wrapped.method("list_pop_back!", [] (WrappedT& v) { v.pop_back(); });
    wrapped.method("list_pop_front!", [] (WrappedT& v) { v.pop_front(); });
    wrapped.method("iteratorbegin", [] (WrappedT& v) { return iterator_wrapper_type<WrappedT>{v.begin()}; });
    wrapped.method("iteratorend", [] (WrappedT& v) { return iterator_wrapper_type<WrappedT>{v.end()}; });
#ifdef JLCXX_HAS_RANGES
    if constexpr(has_less<T>)
    {
      wrapped.method("StdUpperBound", [] (WrappedT& v, const T& val) { return iterator_wrapper_type<WrappedT>{std::ranges::upper_bound(v, val)}; });
      wrapped.method("StdLowerBound", [] (WrappedT& v, const T& val) { return iterator_wrapper_type<WrappedT>{std::ranges::lower_bound(v, val)}; });
      wrapped.method("StdListSort", [] (WrappedT& v) { v.sort(); });
    }
#endif
    wrapped.module().unset_override_module();
  }
};

template<>
struct WrapSTLContainer<std::forward_list> : STLTypeWrapperBase<WrapSTLContainer<std::forward_list>,true>
{
  static std::string name() { return "StdForwardList"; }
  static jl_value_t* supertype() { return (jl_value_t*)jl_any_type; }

  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    using WrappedT = typename TypeWrapperT::type;
    using T = typename WrappedT::value_type;

    wrap_range_based_fill(wrapped);
    wrapped.template constructor<>();
    wrapped.module().set_override_module(stl_module());
    wrapped.method("flist_empty!", [] (WrappedT& v) { v.clear(); });
    wrapped.method("flist_isempty", [] (WrappedT& v) { return v.empty(); });
    wrapped.method("flist_front", [] (WrappedT& v) { return v.front(); });
    wrapped.method("flist_push_front!", [] (WrappedT& v, const T& val) { v.push_front(val); });
    wrapped.method("flist_pop_front!", [] (WrappedT& v) { v.pop_front(); });
    wrapped.method("iteratorbegin", [] (WrappedT& v) { return iterator_wrapper_type<WrappedT>{v.begin()}; });
    wrapped.method("iteratorend", [] (WrappedT& v) { return iterator_wrapper_type<WrappedT>{v.end()}; });
    wrapped.module().unset_override_module();
  }
};

template<template<typename...> class ContainerT, typename T, typename... Args>
struct stl_container_type_factory
{
  using MappedT = ContainerT<T,Args...>;

  static inline jl_datatype_t* julia_type()
  {
    create_if_not_exists<T>();
    assert(!has_julia_type<MappedT>());
    assert(registry().has_current_module());
    jl_datatype_t* jltype = ::jlcxx::julia_type<T>();
    Module& curmod = registry().current_module();
    WrapSTLContainer<ContainerT> wrap;
    wrap.template apply<MappedT>(curmod);
    assert(has_julia_type<MappedT>());
    return stored_type<MappedT>().get_dt();
  }
};

}

template<typename... Args> struct julia_type_factory<std::vector<Args...>>             : stl::stl_container_type_factory<std::vector,Args...> {};
template<typename... Args> struct julia_type_factory<std::valarray<Args...>>           : stl::stl_container_type_factory<std::valarray,Args...> {};
template<typename... Args> struct julia_type_factory<std::deque<Args...>>              : stl::stl_container_type_factory<std::deque,Args...> {};
template<typename... Args> struct julia_type_factory<std::queue<Args...>>              : stl::stl_container_type_factory<std::queue,Args...> {};
template<typename... Args> struct julia_type_factory<std::priority_queue<Args...>>     : stl::stl_container_type_factory<std::priority_queue,Args...> {};
template<typename... Args> struct julia_type_factory<std::stack<Args...>>              : stl::stl_container_type_factory<std::stack,Args...> {};
template<typename... Args> struct julia_type_factory<std::set<Args...>>                : stl::stl_container_type_factory<std::set,Args...> {};
template<typename... Args> struct julia_type_factory<std::unordered_set<Args...>>      : stl::stl_container_type_factory<std::unordered_set,Args...> {};
template<typename... Args> struct julia_type_factory<std::multiset<Args...>>           : stl::stl_container_type_factory<std::multiset,Args...> {};
template<typename... Args> struct julia_type_factory<std::unordered_multiset<Args...>> : stl::stl_container_type_factory<std::unordered_multiset,Args...> {};
template<typename... Args> struct julia_type_factory<std::list<Args...>>               : stl::stl_container_type_factory<std::list,Args...> {};
template<typename... Args> struct julia_type_factory<std::forward_list<Args...>>       : stl::stl_container_type_factory<std::forward_list,Args...> {};

}

#endif
