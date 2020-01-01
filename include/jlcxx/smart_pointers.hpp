#ifndef JLCXX_SMART_POINTER_HPP
#define JLCXX_SMART_POINTER_HPP

#include "module.hpp"
#include "type_conversion.hpp"

namespace jlcxx
{

struct NoSmartOther {};
struct NoSmartBase {};

template<typename T> struct IsSmartPointerType<std::shared_ptr<T>> : std::true_type { };
template<typename T> struct IsSmartPointerType<std::unique_ptr<T>> : std::true_type { };
template<typename T> struct IsSmartPointerType<std::weak_ptr<T>> : std::true_type { };

/// Override to indicate what smart pointer type is a valid constructor argument, e.g. shared_ptr can be used to construct a weak_ptr
template<typename T> struct ConstructorPointerType { typedef NoSmartOther type; };
template<typename T> struct ConstructorPointerType<std::weak_ptr<T>> { typedef std::shared_ptr<T> type; };

namespace smartptr
{

template<typename T>
struct DereferenceSmartPointer
{
  static auto& apply(T& smart_ptr)
  {
    return *smart_ptr;
  }
};

// std::weak_ptr requires a call to lock()
template<typename T>
struct DereferenceSmartPointer<std::weak_ptr<T>>
{
  static auto& apply(std::weak_ptr<T>& smart_ptr)
  {
    return *(smart_ptr.lock());
  }
};

template<typename ToType, typename FromType> struct
ConstructFromOther
{
  static ToType apply(FromType& from_ptr)
  {
    return ToType(from_ptr);
  }
};

// Conversion to base type
template<typename T>
struct ConvertToBase
{
  static NoSmartBase apply(T&)
  {
    static_assert(sizeof(T)==0, "No appropriate specialization for ConvertToBase");
    return NoSmartBase();
  }
};

template<template<typename...> class PtrT, typename T>
struct ConvertToBase<PtrT<T>>
{
  using SuperPtrT = PtrT<supertype<T>>;

  static PtrT<supertype<T>> apply(PtrT<T> &smart_ptr)
  {
    return PtrT<supertype<T>>(smart_ptr);
  }
};

namespace detail
{

template<typename PtrT, typename OtherPtrT>
struct SmartPtrMethods
{
};

template<template<typename...> class PtrT, typename PointeeT, typename OtherPtrT, typename... ExtraArgs>
struct SmartPtrMethods<PtrT<PointeeT, ExtraArgs...>, OtherPtrT>
{
  using WrappedT = PtrT<PointeeT, ExtraArgs...>;

  template<bool B, typename DummyT=void>
  struct ConditionalConstructFromOther
  {
    static void apply(Module& mod)
    {
      mod.method("__cxxwrap_smartptr_construct_from_other", [] (SingletonType<WrappedT>, OtherPtrT& ptr) { return ConstructFromOther<WrappedT, OtherPtrT>::apply(ptr); });
      mod.last_function().set_override_module(get_cxxwrap_module());
    }
  };
  template<typename DummyT> struct ConditionalConstructFromOther<false, DummyT> { static void apply(Module&) {} };

  template<bool B, typename DummyT=void>
  struct ConditionalCastToBase
  {
    static void apply(Module& mod)
    {
      mod.method("__cxxwrap_smartptr_cast_to_base", [] (WrappedT& ptr) { return ConvertToBase<WrappedT>::apply(ptr); });
      mod.last_function().set_override_module(get_cxxwrap_module());
    }
  };
  template<typename DummyT> struct ConditionalCastToBase<false, DummyT> { static void apply(Module&) {} };

  static void apply(Module& mod)
  {
    assert(has_julia_type<WrappedT>());
    ConditionalConstructFromOther<!std::is_same<OtherPtrT, NoSmartOther>::value>::apply(mod);
    ConditionalCastToBase<!std::is_same<PointeeT,supertype<PointeeT>>::value && !std::is_same<std::unique_ptr<PointeeT>, WrappedT>::value>::apply(mod);
  }
};

}

JLCXX_API void set_smartpointer_type(const type_hash_t& hash, TypeWrapper1* new_wrapper);
JLCXX_API TypeWrapper1* get_smartpointer_type(const type_hash_t& hash);

template<template<typename...> class T>
TypeWrapper1 smart_ptr_wrapper(Module& module)
{
  static TypeWrapper1* stored_wrapper = get_smartpointer_type(type_hash<T<int>>());
  if(stored_wrapper == nullptr)
  {
    std::cerr << "Smart pointer type has no wrapper" << std::endl;
    abort();
  }
  return std::move(TypeWrapper1(module, *stored_wrapper));
}

struct WrapSmartPointer
{
  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    using WrappedT = typename TypeWrapperT::type;
    
    wrapped.module().method("__cxxwrap_smartptr_dereference", DereferenceSmartPointer<WrappedT>::apply);
    wrapped.module().last_function().set_override_module(get_cxxwrap_module());
  }
};

template<template<typename...> class PtrT, typename TypeListT>
inline void apply_smart_combination(Module& mod)
{
  smart_ptr_wrapper<PtrT>(mod).template apply_combination<PtrT, TypeListT>(WrapSmartPointer());
}

} // namespace smartptr

template<template<typename...> class T>
TypeWrapper1& add_smart_pointer(Module& mod, const std::string& name)
{
  TypeWrapper1* tw = new TypeWrapper1(mod.add_type<Parametric<TypeVar<1>>>(name, julia_type("SmartPointer", get_cxxwrap_module())));
  smartptr::set_smartpointer_type(type_hash<T<int>>(), tw);
  return *tw;
}

struct SmartPointerTrait {};

template<typename T>
struct MappingTrait<T, typename std::enable_if<IsSmartPointerType<T>::value>::type>
{
  using type = CxxWrappedTrait<SmartPointerTrait>;
};

namespace detail
{

template<typename T>
struct apply_smart_ptr_type
{
};

template<template<typename...> class PtrT, typename T, typename... OtherParamsT>
struct apply_smart_ptr_type<PtrT<T, OtherParamsT...>>
{
  void operator()(Module& curmod)
  {
    smartptr::smart_ptr_wrapper<PtrT>(curmod).template apply<PtrT<T, OtherParamsT...>>(smartptr::WrapSmartPointer());
  }
};

template<typename T>
struct get_pointee
{
};

template<template<typename...> class PtrT, typename T, typename... OtherParamsT>
struct get_pointee<PtrT<T, OtherParamsT...>>
{
  using pointee_t = typename std::remove_const<T>::type;
  using pointer_t = PtrT<pointee_t, OtherParamsT...>;
  using const_pointer_t = PtrT<const pointee_t, OtherParamsT...>;
};

}

template<typename T>
struct julia_type_factory<T, CxxWrappedTrait<SmartPointerTrait>>
{
  static inline jl_datatype_t* julia_type()
  {
    using PointeeT = typename detail::get_pointee<T>::pointee_t;
    using MappedT = typename detail::get_pointee<T>::pointer_t;
    create_if_not_exists<PointeeT>();
    if constexpr(!std::is_same<supertype<PointeeT>, PointeeT>::value)
    {
      create_if_not_exists<typename smartptr::ConvertToBase<MappedT>::SuperPtrT>();
    }
    if(has_julia_type<MappedT>())
    {
      assert((std::is_same<T, typename detail::get_pointee<T>::const_pointer_t>::value));
      return JuliaTypeCache<MappedT>::julia_type();
    }
    assert(registry().has_current_module());
    jl_datatype_t* jltype = ::jlcxx::julia_type<PointeeT>();
    Module& curmod = registry().current_module();
    if(jltype->name->module != curmod.julia_module())
    {
      const std::string tname = julia_type_name(jltype);
      throw std::runtime_error("Smart pointer type with parameter " + tname + " must be defined in the same module as " + tname);
    }
    detail::apply_smart_ptr_type<MappedT>()(curmod);
    smartptr::detail::SmartPtrMethods<MappedT, typename ConstructorPointerType<MappedT>::type>::apply(curmod);
    assert(has_julia_type<MappedT>());
    return JuliaTypeCache<MappedT>::julia_type();
  }
};

}

#endif
