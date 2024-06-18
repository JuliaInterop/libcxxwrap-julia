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

class JLCXX_API StlWrappers
{
private:
  StlWrappers(Module& mod);
  static std::unique_ptr<StlWrappers> m_instance;
  Module& m_stl_mod;
public:
  TypeWrapper1 vector;
  TypeWrapper1 valarray;
  TypeWrapper1 deque;
  TypeWrapper1 queue;
  TypeWrapper1 priority_queue;
  TypeWrapper1 stack;
  TypeWrapper1 set;
  TypeWrapper1 multiset;
  TypeWrapper1 unordered_set;
  TypeWrapper1 unordered_multiset;
  TypeWrapper1 list;
  TypeWrapper1 forward_list;

  static void instantiate(Module& mod);
  static StlWrappers& instance();

  inline jl_module_t* module() const
  {
    return m_stl_mod.julia_module();
  }
};

// Separate per-container functions to split up the compilation over multiple C++ files
void apply_vector(TypeWrapper1& vector);
void apply_valarray(TypeWrapper1& valarray);
void apply_deque(TypeWrapper1& deque);
void apply_queue(TypeWrapper1& queue);
void apply_priority_queue(TypeWrapper1& priority_queue);
void apply_stack(TypeWrapper1& stack);
void apply_set(TypeWrapper1& set);
void apply_multiset(TypeWrapper1& multiset);
void apply_unordered_set(TypeWrapper1& unordered_set);
void apply_unordered_multiset(TypeWrapper1& unordered_multiset);
void apply_list(TypeWrapper1& list);
void apply_forward_list(TypeWrapper1& forward_list);
void apply_shared_ptr();
void apply_weak_ptr();
void apply_unique_ptr();

JLCXX_API StlWrappers& wrappers();

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
void wrap_range_based_algorithms([[maybe_unused]] TypeWrapperT& wrapped)
{
#ifdef JLCXX_HAS_RANGES
  using WrappedT = typename TypeWrapperT::type;
  using T = typename WrappedT::value_type;
  wrapped.module().set_override_module(StlWrappers::instance().module());
  wrapped.method("StdFill", [] (WrappedT& v, const T& val) { std::ranges::fill(v, val); });
  wrapped.module().unset_override_module();
#endif
}

template<typename T>
struct WrapVectorImpl
{
  template<typename TypeWrapperT>
  static void wrap(TypeWrapperT&& wrapped)
  {
    using WrappedT = std::vector<T>;
    
    wrap_range_based_algorithms(wrapped);
    wrapped.module().set_override_module(StlWrappers::instance().module());
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

    wrapped.module().set_override_module(StlWrappers::instance().module());
    wrapped.method("push_back", [] (WrappedT& v, const bool val) { v.push_back(val); });
    wrapped.method("cxxgetindex", [] (const WrappedT& v, cxxint_t i) { return bool(v[i-1]); });
    wrapped.method("cxxsetindex!", [] (WrappedT& v, const bool val, cxxint_t i) { v[i-1] = val; });
    wrapped.module().unset_override_module();
  }
};

struct WrapVector
{
  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    using WrappedT = typename TypeWrapperT::type;
    using T = typename WrappedT::value_type;
    wrapped.module().set_override_module(StlWrappers::instance().module());
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

struct WrapValArray
{
  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    using WrappedT = typename TypeWrapperT::type;
    using T = typename WrappedT::value_type;

    wrap_range_based_algorithms(wrapped); 
    wrapped.template constructor<std::size_t>();
    wrapped.template constructor<const T&, std::size_t>();
    wrapped.template constructor<const T*, std::size_t>();
    wrapped.module().set_override_module(StlWrappers::instance().module());
    wrapped.method("cppsize", &WrappedT::size);
    wrapped.method("resize", [] (WrappedT& v, const cxxint_t s) { v.resize(s); });
    wrapped.method("cxxgetindex", [] (const WrappedT& v, cxxint_t i) -> const T& { return v[i-1]; });
    wrapped.method("cxxgetindex", [] (WrappedT& v, cxxint_t i) -> T& { return v[i-1]; });
    wrapped.method("cxxsetindex!", [] (WrappedT& v, const T& val, cxxint_t i) { v[i-1] = val; });
    wrapped.module().unset_override_module();
  }
};

struct WrapDeque
{
  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    using WrappedT = typename TypeWrapperT::type;
    using T = typename WrappedT::value_type;

    wrap_range_based_algorithms(wrapped);
    wrapped.template constructor<std::size_t>();
    wrapped.module().set_override_module(StlWrappers::instance().module());
    wrapped.method("cppsize", &WrappedT::size);
    wrapped.method("resize", [](WrappedT &v, const cxxint_t s) { v.resize(s); });
    wrapped.method("cxxgetindex", [](const WrappedT& v, cxxint_t i) -> const T& { return v[i - 1]; });
    wrapped.method("cxxsetindex!", [](WrappedT& v, const T& val, cxxint_t i) { v[i - 1] = val; });
    wrapped.method("push_back!", [] (WrappedT& v, const T& val) { v.push_back(val); });
    wrapped.method("push_front!", [] (WrappedT& v, const T& val) { v.push_front(val); });
    wrapped.method("pop_back!", [] (WrappedT& v) { v.pop_back(); });
    wrapped.method("pop_front!", [] (WrappedT& v) { v.pop_front(); });
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
    
    wrapped.module().set_override_module(StlWrappers::instance().module());
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

    wrapped.module().set_override_module(StlWrappers::instance().module());
    wrapped.method("cppsize", &WrappedT::size);
    wrapped.method("q_empty", [] (WrappedT& v) { return v.empty(); });
    wrapped.method("push_back!", [] (WrappedT& v, const bool val) { v.push(val); });
    wrapped.method("front", [] (WrappedT& v) -> bool { return v.front(); });
    wrapped.method("pop_front!", [] (WrappedT& v) { v.pop(); });
    wrapped.module().unset_override_module();
  }
};

struct WrapQueue
{
  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    using WrappedT = typename TypeWrapperT::type;
    using T = typename WrappedT::value_type;
    WrapQueueImpl<T>::wrap(wrapped);
  }
};

struct WrapPriorityQueue
{
  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    using WrappedT = typename TypeWrapperT::type;
    using T = typename WrappedT::value_type;

    wrapped.template constructor<>();
    wrapped.module().set_override_module(StlWrappers::instance().module());
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

struct WrapStack
{
  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    using WrappedT = typename TypeWrapperT::type;
    using T = typename WrappedT::value_type;

    wrapped.template constructor<>();
    wrapped.module().set_override_module(StlWrappers::instance().module());
    wrapped.method("cppsize", &WrappedT::size);
    wrapped.method("stack_isempty", [] (WrappedT& v) { return v.empty(); });
    wrapped.method("stack_push!", [] (WrappedT& v, const T& val) { v.push(val); });
    wrapped.method("stack_top", [] (WrappedT& v) { return v.top(); });
    wrapped.method("stack_pop!", [] (WrappedT& v) { v.pop(); });
    wrapped.module().unset_override_module();
  }
};

struct WrapSetType
{
  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    using WrappedT = typename TypeWrapperT::type;
    using T = typename WrappedT::value_type;

    wrapped.template constructor<>();
    wrapped.module().set_override_module(StlWrappers::instance().module());
    wrapped.method("cppsize", &WrappedT::size);
    wrapped.method("set_insert!", [] (WrappedT& v, const T& val) { v.insert(val); });
    wrapped.method("set_empty!", [] (WrappedT& v) { v.clear(); });
    wrapped.method("set_isempty", [] (WrappedT& v) { return v.empty(); });
    wrapped.method("set_delete!", [] (WrappedT&v, const T& val) { v.erase(val); });
    wrapped.method("set_in", [] (WrappedT& v, const T& val) { return v.count(val) != 0; });
    wrapped.module().unset_override_module();
  }
};

struct WrapMultisetType
{
  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    using WrappedT = typename TypeWrapperT::type;
    using T = typename WrappedT::value_type;

    wrapped.template constructor<>();
    wrapped.module().set_override_module(StlWrappers::instance().module());
    wrapped.method("cppsize", &WrappedT::size);
    wrapped.method("multiset_insert!", [] (WrappedT& v, const T& val) { v.insert(val); });
    wrapped.method("multiset_empty!", [] (WrappedT& v) { v.clear(); });
    wrapped.method("multiset_isempty", [] (WrappedT& v) { return v.empty(); });
    wrapped.method("multiset_delete!", [] (WrappedT&v, const T& val) { v.erase(val); });
    wrapped.method("multiset_in", [] (WrappedT& v, const T& val) { return v.count(val) != 0; });
    wrapped.method("multiset_count", [] (WrappedT& v, const T& val) { return v.count(val); });
    wrapped.module().unset_override_module();
  }
};

struct WrapList
{
  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    using WrappedT = typename TypeWrapperT::type;
    using T = typename WrappedT::value_type;

    wrapped.template constructor<>();
    wrapped.module().set_override_module(StlWrappers::instance().module());
    wrapped.method("cppsize", &WrappedT::size);
    wrapped.method("list_empty!", [] (WrappedT& v) { v.clear(); });
    wrapped.method("list_isempty", [] (WrappedT& v) { return v.empty(); });
    wrapped.method("list_front", [] (WrappedT& v) { return v.front(); });
    wrapped.method("list_back", [] (WrappedT& v) { return v.back(); });
    wrapped.method("list_push_back!", [] (WrappedT& v, const T& val) { v.push_back(val); });
    wrapped.method("list_push_front!", [] (WrappedT& v, const T& val) { v.push_front(val); });
    wrapped.method("list_pop_back!", [] (WrappedT& v) { v.pop_back(); });
    wrapped.method("list_pop_front!", [] (WrappedT& v) { v.pop_front(); });
    wrapped.module().unset_override_module();
  }
};

struct WrapForwardList
{
  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    using WrappedT = typename TypeWrapperT::type;
    using T = typename WrappedT::value_type;

    wrapped.template constructor<>();
    wrapped.module().set_override_module(StlWrappers::instance().module());
    wrapped.method("flist_empty!", [] (WrappedT& v) { v.clear(); });
    wrapped.method("flist_isempty", [] (WrappedT& v) { return v.empty(); });
    wrapped.method("flist_front", [] (WrappedT& v) { return v.front(); });
    wrapped.method("flist_push_front!", [] (WrappedT& v, const T& val) { v.push_front(val); });
    wrapped.method("flist_pop_front!", [] (WrappedT& v) { v.pop_front(); });
    wrapped.module().unset_override_module();
  }
};

template <typename T, typename = void>
struct has_less_than_operator : std::false_type {};

template <typename T>
struct has_less_than_operator<T, std::void_t<decltype(std::declval<T>() < std::declval<T>())>>
    : std::true_type {};

template <typename T>
constexpr bool has_less_than_operator_v = has_less_than_operator<T>::value;

template <typename T, typename = void>
struct is_container : std::false_type {};

template <typename T>
struct is_container<T, std::void_t<typename T::value_type>> : std::true_type {};

template <typename T, typename = void>
struct is_pair : std::false_type {};

template <typename T>
struct is_pair<T, std::void_t<typename T::first_type, typename T::second_type>> : std::true_type {};

template <typename T, typename = void>
struct container_has_less_than_operator : std::false_type {};

template <typename T>
struct container_has_less_than_operator<T, std::enable_if_t<is_container<T>::value>>
    : std::conditional_t<
          container_has_less_than_operator<typename T::value_type>::value,
          std::true_type,
          std::false_type> {};

template <typename T>
struct container_has_less_than_operator<T, std::enable_if_t<is_pair<T>::value>>
    : std::conditional_t<
          container_has_less_than_operator<typename T::first_type>::value &&
              container_has_less_than_operator<typename T::second_type>::value,
          std::true_type,
          std::false_type> {};

template <typename T>
struct container_has_less_than_operator<T, std::enable_if_t<!is_container<T>::value && !is_pair<T>::value>>
    : has_less_than_operator<T> {};

template <typename T, typename = void>
struct is_hashable : std::false_type {};

template <typename T>
struct is_hashable<T, std::void_t<decltype(std::hash<T>{}(std::declval<T>()))>> : std::true_type {};

template<typename T>
inline void apply_stl(jlcxx::Module& mod)
{
  TypeWrapper1(mod, StlWrappers::instance().vector).apply<std::vector<T>>(WrapVector());
  TypeWrapper1(mod, StlWrappers::instance().valarray).apply<std::valarray<T>>(WrapValArray());
  TypeWrapper1(mod, StlWrappers::instance().deque).apply<std::deque<T>>(WrapDeque());
  TypeWrapper1(mod, StlWrappers::instance().queue).apply<std::queue<T>>(WrapQueue());
  TypeWrapper1(mod, StlWrappers::instance().stack).apply<std::stack<T>>(WrapStack());
  if constexpr (container_has_less_than_operator<T>::value)
  {
    TypeWrapper1(mod, StlWrappers::instance().set).apply<std::set<T>>(WrapSetType());
    TypeWrapper1(mod, StlWrappers::instance().multiset).apply<std::multiset<T>>(WrapMultisetType());
    TypeWrapper1(mod, StlWrappers::instance().priority_queue).apply<std::priority_queue<T>>(WrapPriorityQueue());
  }
  if constexpr (is_hashable<T>::value)
  {
    TypeWrapper1(mod, StlWrappers::instance().unordered_set).apply<std::unordered_set<T>>(WrapSetType());
    TypeWrapper1(mod, StlWrappers::instance().unordered_multiset).apply<std::unordered_multiset<T>>(WrapMultisetType());
  }
  TypeWrapper1(mod, StlWrappers::instance().list).apply<std::list<T>>(WrapList());
  TypeWrapper1(mod, StlWrappers::instance().forward_list).apply<std::forward_list<T>>(WrapForwardList());
}

}

template<typename T>
struct julia_type_factory<std::vector<T>>
{
  using MappedT = std::vector<T>;

  static inline jl_datatype_t* julia_type()
  {
    create_if_not_exists<T>();
    assert(!has_julia_type<MappedT>());
    assert(registry().has_current_module());
    jl_datatype_t* jltype = ::jlcxx::julia_type<T>();
    Module& curmod = registry().current_module();
    stl::apply_stl<T>(curmod);
    assert(has_julia_type<MappedT>());
    return stored_type<MappedT>().get_dt();
  }
};

}

#endif
