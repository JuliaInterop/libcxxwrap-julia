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

template<typename PtrT>
inline jl_value_t* box_smart_pointer(PtrT* p)
{
  return boxed_cpp_pointer(p, static_type_mapping<PtrT>::julia_type(), true);
}

template<typename ToType, typename FromType> struct
ConstructFromOther
{
  static ToType* apply(FromType& from_ptr)
  {
    return new ToType(from_ptr);
  }
};

// Conversion to base type
template<typename T>
struct ConvertToBase
{
  static NoSmartBase* apply(T&)
  {
    static_assert(sizeof(T)==0, "No appropriate specialization for ConvertToBase");
    return nullptr;
  }
};

template<template<typename...> class PtrT, typename T>
struct ConvertToBase<PtrT<T>>
{
  static PtrT<supertype<T>>* apply(PtrT<T> &smart_ptr)
  {
    return new PtrT<supertype<T>>(smart_ptr);
  }
};

inline jl_value_t* julia_smartpointer_type()
{
  static jl_value_t* m_ptr_type = (jl_value_t*)julia_type("SmartPointerWithDeref", "CxxWrap");
  return m_ptr_type;
}

namespace detail
{

template<typename PtrT, typename OtherPtrT>
struct BaseMapping
{
};

template<template<typename...> class PtrT, typename PointeeT, typename OtherPtrT, typename... ExtraArgs>
struct BaseMapping<PtrT<PointeeT, ExtraArgs...>, OtherPtrT>
{
  template<bool B, typename DummyT=void>
  struct ConditionalConstructFromOther
  {
    static void apply()
    {
      registry().current_module().method("__cxxwrap_smartptr_construct_from_other", [] (OtherPtrT& ptr) { return box_smart_pointer(ConstructFromOther<PtrT<PointeeT>, OtherPtrT>::apply(ptr)); });
    }
  };
  template<typename DummyT> struct ConditionalConstructFromOther<false, DummyT> { static void apply() {} };

  template<bool B, typename DummyT=void>
  struct ConditionalCastToBase
  {
    static void apply()
    {
      registry().current_module().method("__cxxwrap_smartptr_cast_to_base", [] (PtrT<PointeeT>& ptr) { return box_smart_pointer(ConvertToBase<PtrT<PointeeT>>::apply(ptr)); });
      // Make sure to instantiate the pointer type to the base class
      static_type_mapping<typename std::remove_pointer<decltype(ConvertToBase<PtrT<PointeeT>>::apply(std::declval<PtrT<PointeeT>&>()))>::type>::julia_type();
    }
  };
  template<typename DummyT> struct ConditionalCastToBase<false, DummyT> { static void apply() {} };

  static void instantiate()
  {
    registry().current_module().method("__cxxwrap_smartptr_dereference", DereferenceSmartPointer<PtrT<PointeeT>>::apply);
    ConditionalConstructFromOther<!std::is_same<OtherPtrT, NoSmartOther>::value>::apply();
    ConditionalCastToBase<!std::is_same<PointeeT,supertype<PointeeT>>::value && !std::is_same<std::unique_ptr<PointeeT>, PtrT<PointeeT>>::value>::apply();
  }
};

template<typename PtrT, typename DefaultPtrT, typename T>
inline jl_datatype_t* smart_julia_type()
{
  static jl_datatype_t* wrapped_dt = nullptr;
  static jl_datatype_t* result = nullptr;

  jl_datatype_t* current_dt = static_type_mapping<remove_const_ref<T>>::julia_type();
  if(current_dt != wrapped_dt)
  {
    wrapped_dt = current_dt;
    result = nullptr;
  }

  if(result == nullptr)
  {
    result = (jl_datatype_t*)apply_type(julia_smartpointer_type(), jl_svec2(wrapped_dt, jl_symbol(typeid(DefaultPtrT).name())));
    protect_from_gc(result);

    BaseMapping<PtrT, typename ConstructorPointerType<PtrT>::type>::instantiate();
  }
  return result;
}

template<typename T>
struct SmartJuliaType;

template<template<typename...> class PtrT, typename T>
struct SmartJuliaType<PtrT<T>>
{
  static jl_datatype_t* apply()
  {
    return smart_julia_type<PtrT<T>,PtrT<int>,T>();
  }
};

template<template<typename...> class PtrT, typename T> struct SmartJuliaType<PtrT<const T>>
{
  static jl_datatype_t* apply() { return SmartJuliaType<PtrT<T>>::apply(); }
};

template<typename T>
struct SmartJuliaType<std::unique_ptr<T>>
{
  static jl_datatype_t* apply()
  {
    return smart_julia_type<std::unique_ptr<T>,std::unique_ptr<int>,T>();
  }
};

template<typename T>
struct SmartJuliaType<std::unique_ptr<const T>>
{
  static jl_datatype_t* apply() { return SmartJuliaType<std::unique_ptr<T>>::apply(); }
};

}

struct SmartPointerTrait {};

template<typename T>
struct MappingTrait<T, typename std::enable_if<IsSmartPointerType<T>::value>::type>
{
  using type = SmartPointerTrait;
};

template<typename T>
struct static_type_mapping<T, SmartPointerTrait>
{
  typedef jl_value_t* type;
  static jl_datatype_t* julia_type()
  {
    return detail::SmartJuliaType<T>::apply();
  }
};

template<typename T>
struct ConvertToJulia<T, SmartPointerTrait>
{
  jl_value_t* operator()(T cpp_val) const
  {
    return boxed_cpp_pointer(new T(std::move(cpp_val)), static_type_mapping<T>::julia_type(), true);
  }
};

template<typename T>
struct ConvertToCpp<T, SmartPointerTrait>
{
  T operator()(jl_value_t* julia_val) const
  {
    return *unbox_wrapped_ptr<T>(julia_val);
  }
};

template<typename T>
struct ConvertToJulia<T&, SmartPointerTrait>
{
  jl_value_t* operator()(T& cpp_val) const
  {
    return boxed_cpp_pointer(&cpp_val, static_type_mapping<T>::julia_type(), false);
  }
};

template<typename T>
struct ConvertToCpp<T&, SmartPointerTrait>
{
  T& operator()(jl_value_t* julia_val) const
  {
    return *unbox_wrapped_ptr<T>(julia_val);
  }
};

}

#endif
