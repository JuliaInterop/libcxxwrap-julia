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
      boxed[I] = box<std::tuple_element_t<I,TupleT>>(std::get<I>(tup));
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

struct TupleTrait {};

template<typename... TypesT>
struct TraitSelector<std::tuple<TypesT...>>
{
  using type = TupleTrait;
};

template<typename... TypesT>
struct MappingTrait<std::tuple<TypesT...>, TupleTrait>
{
  using type = TupleTrait;
};

template<typename... TypesT> struct static_type_mapping<std::tuple<TypesT...>, TupleTrait>
{
  using type = jl_value_t*;
};

template<typename... TypesT> struct julia_type_factory<std::tuple<TypesT...>, TupleTrait>
{
  static jl_datatype_t* julia_type()
  {
    (create_if_not_exists<TypesT>(), ...);
    jl_svec_t *params = nullptr;
    jl_datatype_t* result = nullptr;
    JL_GC_PUSH1(&params);
    params = jl_svec(sizeof...(TypesT), jlcxx::julia_type<TypesT>()...);
    result = jl_apply_tuple_type(params);
    JL_GC_POP();
    return result;
  }
};

template<typename... TypesT>
struct ConvertToJulia<std::tuple<TypesT...>, TupleTrait>
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
struct TraitSelector<NTuple<N,T>>
{
  using type = TupleTrait;
};

template<typename N, typename T>
struct MappingTrait<NTuple<N,T>, TupleTrait>
{
  using type = TupleTrait;
};

template<typename N, typename T>
struct static_type_mapping<NTuple<N,T>, TupleTrait>
{
  typedef jl_datatype_t* type;
};

template<typename N, typename T>
struct julia_type_factory<NTuple<N,T>>
{
  static jl_datatype_t* julia_type()
  {
    create_if_not_exists<T>();
    jl_value_t* t[2] = { ::jlcxx::julia_type<T>(), ::jlcxx::julia_type<N>() };
    jl_value_t* type = apply_type((jl_value_t*)jl_vararg_type, t, 2);
    return jl_apply_tuple_type_v(&type, 1);
  }
};

} // namespace jlcxx
#endif
