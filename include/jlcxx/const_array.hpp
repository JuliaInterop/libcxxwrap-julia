#ifndef JLCXX_CONST_ARRAY_HPP
#define JLCXX_CONST_ARRAY_HPP

#include "jlcxx.hpp"
#include "tuple.hpp"

namespace jlcxx
{

using index_t = cxxint_t;

namespace detail
{
  // Helper to make a C++ tuple of longs based on the number of elements
  template<index_t N, typename... TypesT>
  struct LongNTuple
  {
    typedef typename LongNTuple<N-1, index_t, TypesT...>::type type;
  };

  template<typename... TypesT>
  struct LongNTuple<0, TypesT...>
  {
    typedef std::tuple<TypesT...> type;
  };
}

/// Wrap a pointer, providing the Julia array interface for it
/// The parameter N represents the number of dimensions
template<typename T, index_t N>
class ConstArray
{
public:
  typedef typename detail::LongNTuple<N>::type size_t;

  template<typename... SizesT>
  ConstArray(const T* ptr, const SizesT... sizes) :
    m_arr(ptr),
    m_sizes(sizes...)
  {
  }

  T getindex(const cxxint_t i) const
  {
    return m_arr[i-1];
  }

  size_t size() const
  {
    return m_sizes;
  }

  const T* ptr() const
  {
    return m_arr;
  }

private:
  const T* m_arr;
  const size_t m_sizes;
};

template<typename T, typename... SizesT>
ConstArray<T, sizeof...(SizesT)> make_const_array(const T* p, const SizesT... sizes)
{
  return ConstArray<T, sizeof...(SizesT)>(p, sizes...);
}

struct ConstArrayTrait {};

template<typename T, index_t N>
struct TraitSelector<ConstArray<T,N>>
{
  using type = ConstArrayTrait;
};

template<typename T, index_t N>
struct MappingTrait<ConstArray<T,N>, ConstArrayTrait>
{
  using type = ConstArrayTrait;
};

template<typename T, index_t N>
struct ConvertToJulia<ConstArray<T,N>, ConstArrayTrait>
{
  jl_value_t* operator()(const ConstArray<T,N>& arr)
  {
    jl_value_t* result = nullptr;
    jl_value_t* ptr = nullptr;
    jl_value_t* size = nullptr;
    JL_GC_PUSH3(&result, &ptr, &size);
    ptr = box<const T*>(arr.ptr());
    size = convert_to_julia(arr.size());
    result = jl_new_struct(julia_type<ConstArray<T,N>>(), ptr, size);
    JL_GC_POP();
    return result;
  }
};

template<typename T, index_t N>
struct static_type_mapping<ConstArray<T,N>, ConstArrayTrait>
{
  typedef jl_value_t* type;
};

template<typename T, index_t N>
struct julia_type_factory<ConstArray<T,N>, ConstArrayTrait>
{
  static jl_datatype_t* julia_type()
  {
    jl_datatype_t* pdt = (jl_datatype_t*)::jlcxx::julia_type("ConstArray");
    return  (jl_datatype_t*)apply_type((jl_value_t*)pdt, jl_svec2(::jlcxx::julia_type<T>(), box<index_t>(N)));
  }
};

} // namespace jlcxx
#endif
