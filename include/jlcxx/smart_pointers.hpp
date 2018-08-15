#ifndef JLCXX_SMART_POINTER_HPP
#define JLCXX_SMART_POINTER_HPP

#include "module.hpp"
#include "type_conversion.hpp"

namespace jlcxx
{

template<typename T> struct IsSmartPointerType<std::shared_ptr<T>> : std::true_type { };
template<typename T> struct IsSmartPointerType<std::unique_ptr<T>> : std::true_type { };
template<typename T> struct IsSmartPointerType<std::weak_ptr<T>> : std::true_type { };

/// Override to indicate what smart pointer type is a valid constructor argument, e.g. shared_ptr can be used to construct a weak_ptr
template<typename T> struct ConstructorPointerType { typedef void type; };
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

template<typename ToType, typename FromType> struct
ConstructFromOther
{
  static jl_value_t* apply(FromType& from_ptr)
  {
    return boxed_cpp_pointer(new ToType(from_ptr), static_type_mapping<ToType>::julia_type(), true);
  }
};

template<typename ToType>
struct ConstructFromOther<ToType, void>
{
  static jl_value_t* apply(void)
  {
    jl_error("ConstructFromOther not available for this smart pointer type");
    return nullptr;
  }
};

// Conversion to base type
template<typename T>
struct ConvertToBase
{
  static jl_value_t* apply(T&)
  {
    static_assert(sizeof(T)==0, "No appropriate specialization for ConvertToBase");
    return nullptr;
  }
};

template<template<typename...> class PtrT, typename T>
struct ConvertToBase<PtrT<T>>
{
  static jl_value_t* apply(PtrT<T>& smart_ptr)
  {
    if(std::is_same<T,supertype<T>>::value)
    {
      jl_error(("No compile-time type hierarchy specified. Specialize SuperType to get automatic pointer conversion from " + julia_type_name(julia_type<T>()) + " to its base.").c_str());
    }
    return boxed_cpp_pointer(new PtrT<supertype<T>>(smart_ptr), static_type_mapping<PtrT<supertype<T>>>::julia_type(), true);
  }
};

template<typename T>
struct ConvertToBase<std::unique_ptr<T>>
{
  static jl_value_t* apply(std::unique_ptr<T>&)
  {
    jl_error("No convert to base for std::unique_ptr");
    return nullptr;
  }
};

inline jl_value_t* julia_smartpointer_type()
{
  static jl_value_t* m_ptr_type = (jl_value_t*)julia_type("SmartPointerWithDeref", "CxxWrap");
  return m_ptr_type;
}

namespace detail
{

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
    registry().current_module().method("__cxxwrap_smartptr_dereference", DereferenceSmartPointer<PtrT>::apply);
    registry().current_module().method("__cxxwrap_smartptr_construct_from_other", ConstructFromOther<PtrT, typename ConstructorPointerType<PtrT>::type>::apply);
    registry().current_module().method("__cxxwrap_smartptr_cast_to_base", ConvertToBase<PtrT>::apply);
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

template<typename T>
struct static_type_mapping<T, typename std::enable_if<IsSmartPointerType<typename std::remove_reference<T>::type>::value>::type>
{
  typedef jl_value_t* type;
  static jl_datatype_t* julia_type()
  {
    return detail::SmartJuliaType<T>::apply();
  }
};

template<typename T>
struct ConvertToJulia<T, false, false, false, typename std::enable_if<IsSmartPointerType<T>::value>::type>
{
  jl_value_t* operator()(T cpp_val) const
  {
    return boxed_cpp_pointer(new T(std::move(cpp_val)), static_type_mapping<T>::julia_type(), true);
  }
};

template<typename T>
struct ConvertToCpp<T, false, false, false, typename std::enable_if<IsSmartPointerType<T>::value>::type>
{
  T operator()(jl_value_t* julia_val) const
  {
    return *unbox_wrapped_ptr<T>(julia_val);
  }
};

template<typename T>
struct ConvertToJulia<T&, false, false, false, typename std::enable_if<IsSmartPointerType<T>::value && !std::is_const<T>::value>::type>
{
  jl_value_t* operator()(T& cpp_val) const
  {
    return boxed_cpp_pointer(&cpp_val, static_type_mapping<T>::julia_type(), false);
  }
};

template<typename T>
struct ConvertToCpp<T&, false, false, false, typename std::enable_if<IsSmartPointerType<T>::value && !std::is_const<T>::value>::type>
{
  T& operator()(jl_value_t* julia_val) const
  {
    return *unbox_wrapped_ptr<T>(julia_val);
  }
};

}

#endif
