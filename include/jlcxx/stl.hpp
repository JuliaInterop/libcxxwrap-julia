#ifndef JLCXX_STL_HPP
#define JLCXX_STL_HPP

#include <vector>

#include "module.hpp"
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

class StlWrappers
{
public:
  TypeWrapper<Parametric<TypeVar<1>>> vector;

  static void instantiate(Module& mod);
  static StlWrappers& instance();

private:
  StlWrappers(Module& mod);
  static std::unique_ptr<StlWrappers> m_instance;
};

StlWrappers& wrappers();

namespace detail
{

template<typename... T>
constexpr bool has_type = false;

template<typename T1, typename T2, typename... ParametersT>
constexpr bool has_type<T1, ParameterList<T2, ParametersT...>> = has_type<T1, ParameterList<ParametersT...>>;

template<typename T, typename... ParametersT>
constexpr bool has_type<T, ParameterList<T, ParametersT...>> = true;

template<typename T1, typename T2>
constexpr bool has_type<T1, T2, ParameterList<>> = false;

template<bool, typename T1, typename T2>
struct ConditionalAppend
{
};

template<typename T, typename... ParametersT>
struct ConditionalAppend<true, T, ParameterList<ParametersT...>>
{
  using type = ParameterList<ParametersT..., T>;
};

template<typename T, typename... ParametersT>
struct ConditionalAppend<false, T, ParameterList<ParametersT...>>
{
  using type = ParameterList<ParametersT...>;
};

template<typename... T>
struct RemoveDuplicates
{
  using type = ParameterList<>;
};

template<typename T1, typename... ParametersT>
struct RemoveDuplicates<ParameterList<T1,ParametersT...>>
{
  using type = typename RemoveDuplicates<ParameterList<T1>, ParameterList<ParametersT...>>::type;
};

template<typename ResultT, typename T1, typename... ParametersT>
struct RemoveDuplicates<ResultT, ParameterList<T1,ParametersT...>>
{
  using type = typename RemoveDuplicates<typename ConditionalAppend<!has_type<T1,ResultT>,T1,ResultT>::type, ParameterList<ParametersT...>>::type;
};

template<typename ResultT>
struct RemoveDuplicates<ResultT, ParameterList<>>
{
  using type = ResultT;
};

template<typename T> using remove_duplicates = typename RemoveDuplicates<T>::type;

}

using stltypes = detail::remove_duplicates<ParameterList
<
  bool,
  int,
  double,
  float,
  short,
  unsigned int,
  unsigned char,
  //int64_t,
  //uint64_t,
  long,
  long long,
  unsigned long,
  wchar_t,
  void*,
  char,
  std::string,
  std::wstring
>>;

template<typename TypeWrapperT>
void wrap_common(TypeWrapperT& wrapped)
{
  using WrappedT = typename TypeWrapperT::type;
  using T = typename WrappedT::value_type;
  wrapped.method("cppsize", &WrappedT::size);
  wrapped.method("resize", [] (WrappedT& v, const int_t s) { v.resize(s); });
  wrapped.method("append", [] (WrappedT& v, jlcxx::ArrayRef<T> arr)
  {
    const std::size_t addedlen = arr.size();
    v.reserve(v.size() + addedlen);
    for(size_t i = 0; i != addedlen; ++i)
    {
      v.push_back(arr[i]);
    }
  });
}

template<typename T>
struct WrapVectorImpl
{
  template<typename TypeWrapperT>
  static void wrap(TypeWrapperT&& wrapped)
  {
    using WrappedT = std::vector<T>;
    
    wrap_common(wrapped);
    wrapped.method("push_back", static_cast<void (WrappedT::*)(const T&) >(&WrappedT::push_back));
    wrapped.method("getindex", [] (const WrappedT& v, int_t i) { return v[i-1]; });
    wrapped.method("setindex!", [] (WrappedT& v, const T& val, int_t i) { v[i-1] = val; });
  }
};

template<>
struct WrapVectorImpl<bool>
{
  template<typename TypeWrapperT>
  static void wrap(TypeWrapperT&& wrapped)
  {
    using WrappedT = std::vector<bool>;

    wrap_common(wrapped);
    wrapped.method("push_back", [] (WrappedT& v, const bool val) { v.push_back(val); });
    wrapped.method("getindex", [] (const WrappedT& v, int_t i) { return bool(v[i-1]); });
    wrapped.method("setindex!", [] (WrappedT& v, const bool val, int_t i) { v[i-1] = val; });
  }
};

struct WrapVector
{
  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    using WrappedT = typename TypeWrapperT::type;
    using T = typename WrappedT::value_type;
    WrapVectorImpl<T>::wrap(wrapped);
  }
};

template<typename T>
inline void register_vector_type()
{
  assert(registry().has_current_module());
  jl_datatype_t* jltype = julia_type<T>();
  if(jltype->name->module != registry().current_module().julia_module())
  {
    const std::string tname = julia_type_name(jltype);
    throw std::runtime_error("Type for std::vector<" + tname + "> must be defined in the same module as " + tname);
  }
  wrappers().vector.apply<std::vector<T>>(WrapVector());
}

}

}

#endif
