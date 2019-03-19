#ifndef JLCXX_TUPLE_HPP
#define JLCXX_TUPLE_HPP

#include <tuple>

#include "type_conversion.hpp"

namespace jlcxx
{

namespace detail
{
  template<std::size_t I, std::size_t N>
  struct AppendTupleValues
  {
    template<typename TupleT>
    static void apply(jl_value_t** boxed, const TupleT& tup)
    {
      boxed[I] = box(std::get<I>(tup));
      AppendTupleValues<I+1, std::tuple_size<TupleT>::value>::apply(boxed, tup);
    }
  };

  template<std::size_t N>
  struct AppendTupleValues<N,N>
  {
    template<typename TupleT>
    static void apply(jl_value_t**, const TupleT&)
    {
    }
  };

  template<typename TupleT>
  jl_value_t* new_jl_tuple(const TupleT& tp)
  {
    jl_value_t* result = nullptr;
    jl_datatype_t* concrete_dt = nullptr;
    JL_GC_PUSH2(&result, &concrete_dt);
    {
      constexpr std::size_t tup_sz = std::tuple_size<TupleT>::value;
      jl_value_t** args;
      JL_GC_PUSHARGS(args, tup_sz);
      detail::AppendTupleValues<0, tup_sz>::apply(args, tp);
      {
        jl_value_t** concrete_types;
        JL_GC_PUSHARGS(concrete_types, tup_sz);
        for(std::size_t i = 0; i != tup_sz; ++i)
        {
          concrete_types[i] = jl_typeof(args[i]);
        }
        concrete_dt = jl_apply_tuple_type_v(concrete_types, tup_sz);
        JL_GC_POP();
      }
      result = jl_new_structv(concrete_dt, args, tup_sz);
      JL_GC_POP();
    }
    JL_GC_POP();
    return result;
  }
}

template<typename... TypesT> struct static_type_mapping<std::tuple<TypesT...>>
{
  typedef jl_value_t* type;

  static jl_datatype_t* julia_type()
  {
    static jl_datatype_t* tuple_type = nullptr;
    if(tuple_type == nullptr)
    {
      jl_svec_t* params = nullptr;
      JL_GC_PUSH2(&tuple_type, &params);
      params = jl_svec(sizeof...(TypesT), jlcxx::julia_type<TypesT>()...);
      tuple_type = jl_apply_tuple_type(params);
      protect_from_gc(tuple_type);
      JL_GC_POP();
    }
    return tuple_type;
  }
};

template<typename... TypesT>
struct ConvertToJulia<std::tuple<TypesT...>>
{
  jl_value_t* operator()(const std::tuple<TypesT...>& tp)
  {
    return detail::new_jl_tuple(tp);
  }
};

// Wrap NTuple type
template<typename N, typename T>
struct NTuple
{
};

template<typename N, typename T>
struct static_type_mapping<NTuple<N,T>>
{
  typedef jl_datatype_t* type;
  static jl_datatype_t* julia_type()
  {
    jl_datatype_t* dt = nullptr;
    if(dt == nullptr)
    {
      dt = (jl_datatype_t*)jl_apply_tuple_type(jl_svec1(apply_type((jl_value_t*)jl_vararg_type, jl_svec2(static_type_mapping<T>::julia_type(), static_type_mapping<N>::julia_type()))));
      protect_from_gc(dt);
    }
    return dt;
  }
};

} // namespace jlcxx
#endif
