#ifndef JLCXX_TYPE_CONVERSION_HPP
#define JLCXX_TYPE_CONVERSION_HPP

#include "julia_headers.hpp"

#include <complex>
#include <map>
#include <memory>
#include <stack>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <type_traits>
#include <iostream>

#include "jlcxx_config.hpp"

namespace jlcxx
{

namespace detail
{
  template<std::size_t N>
  struct IndexT
  {
    using type = int_t;
  };

  template<>
  struct IndexT<8>
  {
    using type = int64_t; // make sure index_t maps to Int64 and not CxxInt64
  };
}

using cxxint_t = typename detail::IndexT<sizeof(long)>::type;

namespace detail
{
  template<bool, typename T1, typename T2>
  struct StaticIf;

  // non-bits
  template<typename T1, typename T2>
  struct StaticIf<false, T1, T2>
  {
    typedef T2 type;
  };

  // bits type
  template<typename T1, typename T2>
  struct StaticIf<true, T1, T2>
  {
    typedef T1 type;
  };
}

JLCXX_API void protect_from_gc(jl_value_t* v);
JLCXX_API void unprotect_from_gc(jl_value_t* v);
JLCXX_API void cxx_root_scanner(int);

template<typename T>
inline void protect_from_gc(T* x)
{
  protect_from_gc((jl_value_t*)x);
}

template<typename T>
inline void unprotect_from_gc(T* x)
{
  unprotect_from_gc((jl_value_t*)x);
}

/// Get the symbol name correctly depending on Julia version
inline std::string symbol_name(jl_sym_t* symbol)
{
  return std::string(jl_symbol_name(symbol));
}

inline std::string module_name(jl_module_t* mod)
{
  return symbol_name(mod->name);
}

/// Backwards-compatible apply_type
JLCXX_API jl_value_t* apply_type(jl_value_t* tc, jl_svec_t* params);

//
JLCXX_API jl_value_t* apply_type(jl_value_t* tc, jl_value_t **params, size_t n);
JLCXX_API jl_datatype_t* apply_type(jl_value_t* tc, jl_datatype_t *type);


/// Get the type from a global symbol
JLCXX_API jl_value_t* julia_type(const std::string& name, const std::string& module_name = "");
JLCXX_API jl_value_t* julia_type(const std::string& name, jl_module_t* mod);

/// Backwards-compatible apply_array_type
template<typename T>
inline jl_value_t* apply_array_type(T* type, std::size_t dim)
{
  return jl_apply_array_type((jl_value_t*)type, dim);
}

/// Check if we have a string
inline bool is_julia_string(jl_value_t* v)
{
  return jl_is_string(v);
}

inline const char* julia_string(jl_value_t* v)
{
    return jl_string_ptr(v);
}

inline std::string julia_type_name(jl_value_t* dt)
{
  if(jl_is_unionall(dt))
  {
    jl_unionall_t* ua = (jl_unionall_t*)dt;
    return jl_symbol_name(ua->var->name);
  }
  return jl_typename_str(dt);
}

inline std::string julia_type_name(jl_datatype_t* dt)
{
  return julia_type_name((jl_value_t*)dt);
}

// Specialize to indicate direct Julia supertype in a smart-pointer compatible way i.e. using this to define supertypes
// will make conversion to a smart pointer of the base type work like in C++
template<typename T>
struct SuperType
{
  typedef T type;
};

template<typename T> using supertype = typename SuperType<T>::type;

/// Remove reference and const from a type
template<typename T> using remove_const_ref = typename std::remove_const<typename std::remove_reference<T>::type>::type;

/// Indicate if a type is a smart pointer
template<typename T> struct IsSmartPointerType
{
  static constexpr bool value = false;
};

/// Equivalent of the basic C++ type layout in Julia
struct WrappedCppPtr {
  void* voidptr;
};

/// Store a boxed Julia value together with the C++ type
template<typename T>
struct BoxedValue
{
  operator jl_value_t*() const { return value; }
  jl_value_t* value;
};

template<typename CppT>
inline CppT* extract_pointer(const WrappedCppPtr& p)
{
  return reinterpret_cast<CppT*>(p.voidptr);
}

template<typename CppT>
inline CppT* extract_pointer_nonull(const WrappedCppPtr& p)
{
  if(p.voidptr == nullptr)
  {
    std::stringstream errorstr("");
    errorstr << "C++ object of type " << typeid(CppT).name() << " was deleted";
    throw std::runtime_error(errorstr.str());
  }
  return extract_pointer<CppT>(p);
}

namespace detail
{
  template<typename T>
  struct unused_type
  {
  };

  template<typename T1, typename T2>
  struct DefineIfDifferent
  {
    typedef T1 type;
  };

  template<typename T>
  struct DefineIfDifferent<T,T>
  {
    typedef unused_type<T> type;
  };

  template<typename T1, typename T2> using define_if_different = typename DefineIfDifferent<T1,T2>::type;
}

// By default, fundamental and "POD" types are mapped directly
template<typename T>
struct IsMirroredType : std::bool_constant<(!std::is_class<T>::value || (std::is_standard_layout<T>::value && std::is_trivial<T>::value)) && !IsSmartPointerType<T>::value>
{
};

struct NoMappingTrait {}; // no mapping, C++ type = Julia type by default
template<typename T> struct CxxWrappedTrait {}; // types added using add_type. T is a "sub-trait" to allow further specialization for e.g. smart pointers
struct WrappedPtrTrait {}; // By default pointers are wrapped
struct DirectPtrTrait {}; // Some pointers are returned directly, e.g. jl_value_t*

struct NoCxxWrappedSubtrait {};

// Helper to avoid ambiguous specializations
template<typename T> struct TraitSelector
{
  using type = void;
};

template<typename T, typename Enable=typename TraitSelector<T>::type>
struct MappingTrait
{
  using type = NoMappingTrait;
};

template<typename T>
struct MappingTrait<T&>
{
  using type = WrappedPtrTrait;
};

template<typename T>
struct MappingTrait<T*>
{
  using type = WrappedPtrTrait;
};

template<>
struct MappingTrait<jl_value_t*>
{
  using type = DirectPtrTrait;
};

template<>
struct MappingTrait<jl_datatype_t*>
{
  using type = DirectPtrTrait;
};

template<>
struct MappingTrait<void*>
{
  using type = DirectPtrTrait;
};

template<typename T>
struct MappingTrait<T, typename std::enable_if<!IsMirroredType<T>::value && !IsSmartPointerType<T>::value>::type>
{
  using type = CxxWrappedTrait<NoCxxWrappedSubtrait>;
};

template<typename T> using mapping_trait = typename MappingTrait<T>::type;

/// Static mapping base template, just passing through
template<typename SourceT, typename TraitT=mapping_trait<SourceT>>
struct static_type_mapping
{
  using type = SourceT;
};

template<typename SourceT, typename SubTraitT>
struct static_type_mapping<SourceT, CxxWrappedTrait<SubTraitT>>
{
  using type = WrappedCppPtr;
};

template<typename SourceT>
struct static_type_mapping<SourceT*, DirectPtrTrait>
{
  using type = SourceT*;
};

/// Pointers are a pointer to the equivalent C type
template<typename SourceT>
struct static_type_mapping<SourceT*, WrappedPtrTrait>
{
  using type = WrappedCppPtr;
};

/// References are pointers
template<typename SourceT>
struct static_type_mapping<SourceT&>
{
  using type = WrappedCppPtr;
};

/// Boxed values map to jl_value_t*
template<typename T>
struct static_type_mapping<BoxedValue<T>>
{
  using type = jl_value_t*;
};

template<typename T> using static_julia_type = typename static_type_mapping<T>::type;

// Store a data type pointer, ensuring GC safety
struct CachedDatatype
{
  explicit CachedDatatype() : m_dt(nullptr) {}
  explicit CachedDatatype(jl_datatype_t* dt, bool protect = true)
  {
    set_dt(dt,protect);
  }

  void set_dt(jl_datatype_t* dt, bool protect = true)
  {
    m_dt = dt;
    if(m_dt != nullptr && protect)
    {
      protect_from_gc(m_dt);
    }
  }

  jl_datatype_t* get_dt()
  {
    return m_dt;
  }
  
private:
  jl_datatype_t* m_dt = nullptr;
};

// Work around the fact that references aren't part of the typeid result
using type_hash_t = std::pair<std::size_t, std::size_t>;

namespace detail
{

template<typename T>
struct TypeHash
{
  static inline type_hash_t value()
  {
    return std::make_pair(typeid(T).hash_code(), std::size_t(0));
  }
};

template<typename T>
struct TypeHash<T&>
{
  static inline type_hash_t value()
  {
    return std::make_pair(typeid(T).hash_code(), std::size_t(1));
  }
};

template<typename T>
struct TypeHash<const T&>
{
  static inline type_hash_t value()
  {
    return std::make_pair(typeid(T).hash_code(), std::size_t(2));
  }
};

}

template<typename T>
inline type_hash_t type_hash()
{
  return detail::TypeHash<T>::value();
}

JLCXX_API std::map<type_hash_t, CachedDatatype>& jlcxx_type_map();

/// Store the Julia datatype linked to SourceT
template<typename SourceT>
class JuliaTypeCache
{
public:

  static inline jl_datatype_t* julia_type()
  {
    const auto result = jlcxx_type_map().find(type_hash<SourceT>());
    if(result == jlcxx_type_map().end())
    {
      throw std::runtime_error("Type " + std::string(typeid(SourceT).name()) + " has no Julia wrapper");
    }
    return result->second.get_dt();
  }

  static inline void set_julia_type(jl_datatype_t* dt, bool protect = true)
  {
    const auto insresult = jlcxx_type_map().insert(std::make_pair(type_hash<SourceT>(), CachedDatatype(dt, protect)));
    if(!insresult.second)
    {
      std::cout << "Warning: Type " << typeid(SourceT).name() << " already had a mapped type set as " << julia_type_name(insresult.first->second.get_dt()) << " using hash " << insresult.first->first.first << " and const-ref indicator " << insresult.first->first.second << std::endl;
      return;
    }
  }

  static inline bool has_julia_type()
  {
    const std::size_t nb_hits = jlcxx_type_map().count(type_hash<SourceT>());
    return nb_hits != 0;
  }
};

template<typename T>
void set_julia_type(jl_datatype_t* dt, bool protect = true)
{
  JuliaTypeCache<typename std::remove_const<T>::type>::set_julia_type(dt, protect);
}

/// Store the Julia datatype linked to SourceT
template<typename SourceT, typename TraitT=mapping_trait<SourceT>>
class julia_type_factory
{
public:
  static inline jl_datatype_t* julia_type()
  {
    throw std::runtime_error(std::string("No appropriate factory for type ") + typeid(SourceT).name());
    return nullptr;
  }
};

/// Get the julia data type associated with T
template<typename T>
inline jl_datatype_t* julia_type()
{
  using nonconst_t = typename std::remove_const<T>::type;
  static jl_datatype_t* dt = JuliaTypeCache<nonconst_t>::julia_type();
  return dt;
}

/// Check if a type is registered
template <typename T>
bool has_julia_type()
{
  using nonconst_t = typename std::remove_const<T>::type;
  return JuliaTypeCache<nonconst_t>::has_julia_type();
}

/// Create the julia type associated with the given C++ type
template <typename T>
void create_julia_type()
{
  using nonconst_t = typename std::remove_const<T>::type;
  jl_datatype_t* result = julia_type_factory<nonconst_t>::julia_type();
  if(!has_julia_type<nonconst_t>())
  {
    set_julia_type<nonconst_t>(result);
  }
}

template<typename T>
void create_if_not_exists()
{
  using nonconst_t = typename std::remove_const<T>::type;
  static bool exists = false;
  if(!exists)
  {
    if(!has_julia_type<nonconst_t>())
    {
      create_julia_type<nonconst_t>();
    }
    exists = true;
  }
}

namespace detail
{
  // Gets the dynamic type to put inside a pointer, which is the normal dynamic type for normal types, or the base type for wrapped types
  template<typename T, typename TraitT=mapping_trait<T>>
  struct GetBaseT
  {
    static inline jl_datatype_t* type()
    {
      return julia_type<T>();
    }
  };

  template<typename T, typename SubTraitT>
  struct GetBaseT<T,CxxWrappedTrait<SubTraitT>>
  {
    static inline jl_datatype_t* type()
    {
      return julia_type<T>()->super;
    }
  };
}

// Returns T itself for normal types, or the supertype for wrapped types, e.g. Foo instead of FooAllocated
template<typename T>
inline jl_datatype_t* julia_base_type()
{
  create_if_not_exists<T>();
  return detail::GetBaseT<T>::type();
}

// Mapping for const references
template<typename SourceT>
struct julia_type_factory<const SourceT&>
{
  static inline jl_datatype_t* julia_type()
  {
    return apply_type(jlcxx::julia_type("ConstCxxRef"), julia_base_type<SourceT>());
  }
};

// Mapping for mutable references
template<typename SourceT>
struct julia_type_factory<SourceT&>
{
  static inline jl_datatype_t* julia_type()
  {
    return apply_type(jlcxx::julia_type("CxxRef"), julia_base_type<SourceT>());
  }
};

// Mapping for const pointers
template<typename SourceT>
struct julia_type_factory<const SourceT*>
{
  static inline jl_datatype_t* julia_type()
  {
    return apply_type(jlcxx::julia_type("ConstCxxPtr"), julia_base_type<SourceT>());
  }
};

// Mapping for mutable pointers
template<typename SourceT>
struct julia_type_factory<SourceT*>
{
  static inline jl_datatype_t* julia_type()
  {
    return apply_type(jlcxx::julia_type("CxxPtr"), julia_base_type<SourceT>());
  }
};

template<>
struct julia_type_factory<void*>
{
  static inline jl_datatype_t* julia_type()
  {
    return jl_voidpointer_type;
  }
};

template<>
struct julia_type_factory<jl_datatype_t*>
{
  static inline jl_datatype_t* julia_type()
  {
    return jl_any_type;
  }
};

template<>
struct julia_type_factory<jl_value_t*>
{
  static inline jl_datatype_t* julia_type()
  {
    return jl_any_type;
  }
};

/// Base class to specialize for conversion to C++
template<typename CppT, typename TraitT=mapping_trait<CppT>>
struct ConvertToCpp
{
  template<typename JuliaT>
  CppT* operator()(JuliaT&&) const
  {
    static_assert(sizeof(CppT)==0, "No appropriate specialization for ConvertToCpp");
    return nullptr; // not reached
  }
};

/// Conversion to C++
template<typename CppT, typename JuliaT>
inline CppT convert_to_cpp(JuliaT julia_val)
{
  return ConvertToCpp<CppT>()(julia_val);
}

template<typename T, typename TraitT=mapping_trait<T>>
struct JuliaReturnType
{
  inline static std::pair<jl_datatype_t*,jl_datatype_t*> value()
  {
    return std::make_pair(julia_type<T>(),julia_type<T>());
  }
};

template<typename T, typename SubTraitT>
struct JuliaReturnType<T, CxxWrappedTrait<SubTraitT>>
{
  inline static std::pair<jl_datatype_t*,jl_datatype_t*> value()
  {
    assert(has_julia_type<T>());
    return std::make_pair(jl_any_type,julia_type<T>());
  }
};

template<typename T>
struct JuliaReturnType<BoxedValue<T>>
{
  inline static std::pair<jl_datatype_t*,jl_datatype_t*> value()
  {
    return std::make_pair(jl_any_type,julia_type<T>());
  }
};

// The Julia return type is a pair of the type needed for ccall and the return type assert declared in the wrapped function
template<typename T>
inline std::pair<jl_datatype_t*,jl_datatype_t*> julia_return_type()
{
  create_if_not_exists<T>();
  return JuliaReturnType<T>::value();
}

// Needed for Visual C++, static members are different in each DLL
extern "C" JLCXX_API jl_module_t* get_cxxwrap_module();

namespace detail
{
  inline jl_value_t* get_finalizer()
  {
    static jl_value_t* finalizer = jl_get_function(get_cxxwrap_module(), "delete");
    return finalizer;
  }
}

/// Wrap a C++ pointer in a Julia type that contains a single void pointer field, returning the result as an any
template<typename T>
BoxedValue<T> boxed_cpp_pointer(T* cpp_ptr, jl_datatype_t* dt, bool add_finalizer)
{
  assert(jl_is_concrete_type((jl_value_t*)dt));
  assert(jl_datatype_nfields(dt) == 1);
  assert(jl_is_cpointer_type(jl_field_type(dt,0)));
  assert(jl_datatype_size(jl_field_type(dt,0)) == sizeof(T*));

  jl_value_t *result = jl_new_struct_uninit(dt);
  JL_GC_PUSH1(&result);
  memcpy((void*)result, &cpp_ptr, sizeof(T*));

  if(add_finalizer)
  {
    jl_gc_add_finalizer(result, detail::get_finalizer());
  }
  
  JL_GC_POP();
  return {result};
}

/// Transfer ownership of a regular pointer to Julia
template<typename T>
BoxedValue<T> julia_owned(T* cpp_ptr)
{
  static_assert(!std::is_fundamental<T>::value, "Ownership can't be transferred for fundamental types");
  const bool finalize = true;
  return boxed_cpp_pointer(cpp_ptr, julia_type<T>(), finalize);
}

/// Base class to specialize for conversion to Julia
// C++ wrapped types are in fact always returned as a pointer wrapped in a struct, so to avoid memory management issues with the wrapper itself
// we always return the wrapping struct by value
template<typename T, typename TraitT=mapping_trait<T>>
struct ConvertToJulia
{
  template<typename CppT>
  jl_value_t* operator()(CppT&& cpp_val) const
  {
    static_assert(std::is_same<static_julia_type<T>, WrappedCppPtr>::value, "No appropriate specialization for ConvertToJulia");
    static_assert(std::is_class<T>::value, "Need class type for conversion");
    return julia_owned(new T(std::move(cpp_val)));
  }
};

template<typename T>
struct ConvertToJulia<T&, WrappedPtrTrait>
{
  WrappedCppPtr operator()(T& cpp_val) const
  {
    return {reinterpret_cast<void*>(const_cast<typename std::remove_const<T>::type*>(&cpp_val))};
  }
};

template<typename T>
struct ConvertToJulia<T*, WrappedPtrTrait>
{
  WrappedCppPtr operator()(T* cpp_val) const
  {
    return {reinterpret_cast<void*>(const_cast<typename std::remove_const<T>::type*>(cpp_val))};
  }
};

template<typename T>
struct ConvertToJulia<T*, DirectPtrTrait>
{
  T* operator()(T* cpp_val) const
  {
    return cpp_val;
  }
};

// Fundamental type
template<typename T>
struct ConvertToJulia<T, NoMappingTrait>
{
  T operator()(const T& cpp_val) const
  {
    return cpp_val;
  }
};

// Complex types fail on Windows
#ifdef _WIN32

template<typename T>
struct JlCxxComplex
{
  T a;
  T b;
};

template<typename NumberT>
struct ConvertToJulia<std::complex<NumberT>, NoMappingTrait>
{
  JlCxxComplex<NumberT> operator()(const std::complex<NumberT>& cpp_val) const
  {
    return { cpp_val.real(), cpp_val.imag() };
  }
};
#endif

/// Conversion to the statically mapped target type.
template<typename T>
inline auto convert_to_julia(T&& cpp_val) -> decltype(ConvertToJulia<T>()(std::forward<T>(cpp_val)))
{
  return ConvertToJulia<T>()(std::forward<T>(cpp_val));
}

template<typename CppT, typename JuliaT>
struct BoxValue
{
  inline jl_value_t* operator()(CppT)
  {
    static_assert(sizeof(CppT) == 0, "Unimplemented BoxValue in jlcxx");
    return nullptr;
  }
};

// Boxing of types that map to the same type in Julia
template<typename T>
struct BoxValue<T,T>
{
  inline jl_value_t* operator()(T cppval)
  {
    return jl_new_bits((jl_value_t*)julia_type<T>(), &cppval);
  }

  inline jl_value_t* operator()(T cppval, jl_value_t* dt)
  {
    return jl_new_bits(dt, &cppval);
  }
};

// Already boxed types
template<typename CppT>
struct BoxValue<CppT,jl_value_t*>
{
  inline jl_value_t* operator()(CppT cppval)
  {
    return (jl_value_t*)convert_to_julia(cppval);
  }
};
template<typename CppT>
struct BoxValue<BoxedValue<CppT>,jl_value_t*>
{
  inline jl_value_t* operator()(const BoxedValue<CppT>& v)
  {
    return v;
  }
};

// Pass-through for jl_value_t*
template<>
struct BoxValue<jl_value_t*,jl_value_t*>
{
  inline jl_value_t* operator()(jl_value_t* cppval)
  {
    return cppval;
  }
};

template<>
struct BoxValue<jl_datatype_t*,jl_datatype_t*>
{
  inline jl_value_t* operator()(jl_datatype_t* cppval)
  {
    return (jl_value_t*)cppval;
  }
};

// Box an automatically converted value
template<typename CppT>
struct BoxValue<CppT,WrappedCppPtr>
{
  inline BoxedValue<CppT> operator()(CppT cppval)
  {
    return boxed_cpp_pointer(new CppT(cppval), julia_type<CppT>(), true);
  }
};
template<typename CppT>
struct BoxValue<CppT&,WrappedCppPtr>
{
  inline BoxedValue<CppT&> operator()(CppT& cppval)
  {
    return {boxed_cpp_pointer(&cppval, julia_type<CppT&>(), false).value};
  }
};
template<typename CppT>
struct BoxValue<CppT*,WrappedCppPtr>
{
  inline BoxedValue<CppT*> operator()(CppT* cppval)
  {
    return {boxed_cpp_pointer(cppval, julia_type<CppT*>(), false).value};
  }
};

template<typename CppT, typename ArgT>
inline auto box(ArgT&& cppval)
{
  return BoxValue<CppT, static_julia_type<CppT>>()(std::forward<ArgT>(cppval));
}

template<typename CppT, typename ArgT>
inline auto box(ArgT&& cppval, jl_value_t* dt)
{
  return BoxValue<CppT, static_julia_type<CppT>>()(std::forward<ArgT>(cppval), dt);
}

template<typename CppT, typename JuliaT>
struct UnboxValue
{
  inline CppT operator()(jl_value_t*)
  {
    static_assert(sizeof(CppT) == 0, "Unimplemented UnboxValue in jlcxx");
    return CppT();
  }
};

namespace detail
{
  template<typename CppT>
  CppT* unbox_any(jl_value_t* juliaval)
  {
    return reinterpret_cast<CppT*>(jl_data_ptr(juliaval));
  }

  template<typename CppT>
  CppT* unboxed_wrapped_pointer(jl_value_t* juliaval)
  {
    return reinterpret_cast<CppT*>(unbox_any<WrappedCppPtr>(juliaval)->voidptr);
  }
}

template<typename CppT>
struct UnboxValue<CppT,WrappedCppPtr>
{
  inline CppT operator()(jl_value_t* juliaval)
  {
    return *detail::unboxed_wrapped_pointer<CppT>(juliaval);
  }
};

template<typename CppT>
struct UnboxValue<CppT*,WrappedCppPtr>
{
  inline CppT* operator()(jl_value_t* juliaval)
  {
    return detail::unboxed_wrapped_pointer<CppT>(juliaval);
  }
};

template<typename CppT>
struct UnboxValue<CppT&,WrappedCppPtr>
{
  inline CppT& operator()(jl_value_t* juliaval)
  {
    return *detail::unboxed_wrapped_pointer<CppT>(juliaval);
  }
};

template<typename CppT>
struct UnboxValue<CppT,CppT>
{
  inline CppT operator()(jl_value_t* juliaval)
  {
    return *detail::unbox_any<CppT>(juliaval);
  }
};

template<typename CppT>
inline CppT unbox(jl_value_t* juliaval)
{
  return UnboxValue<CppT, static_julia_type<CppT>>()(juliaval);
}

// Fundamental type conversion
template<typename CppT>
struct ConvertToCpp<CppT, NoMappingTrait>
{
  inline CppT operator()(CppT julia_val) const
  {
    return julia_val;
  }
};

/// Conversion of pointer types
template<typename CppT>
struct ConvertToCpp<CppT*, WrappedPtrTrait>
{
  inline CppT* operator()(WrappedCppPtr julia_val) const
  {
    return extract_pointer<CppT>(julia_val);
  }
};

template<typename CppT>
struct ConvertToCpp<CppT*, DirectPtrTrait>
{
  inline CppT* operator()(CppT* julia_val) const
  {
    return julia_val;
  }
};

/// Conversion of reference types
template<typename CppT>
struct ConvertToCpp<CppT&, WrappedPtrTrait>
{
  inline CppT& operator()(WrappedCppPtr julia_val) const
  {
    return *extract_pointer_nonull<CppT>(julia_val);
  }
};

template<typename CppT, typename SubTraitT>
struct ConvertToCpp<CppT, CxxWrappedTrait<SubTraitT>>
{
  inline CppT operator()(WrappedCppPtr julia_val) const
  {
    return *extract_pointer_nonull<CppT>(julia_val);
  }
};

/// Represent a Julia TypeVar in the template parameter list
template<int I>
struct TypeVar
{
  static constexpr int value = I;

  static jl_tvar_t* tvar()
  {
    static jl_tvar_t* this_tvar = build_tvar();
    return this_tvar;
  }

  static jl_tvar_t* build_tvar()
  {
    jl_tvar_t* result = jl_new_typevar(jl_symbol((std::string("T") + std::to_string(I)).c_str()), (jl_value_t*)jl_bottom_type, (jl_value_t*)jl_any_type);
    protect_from_gc(result);
    return result;
  }
};

template<int I>
struct static_type_mapping<TypeVar<I>>
{
  typedef jl_tvar_t* type;
};

template<int I>
struct julia_type_factory<TypeVar<I>>
{
  typedef jl_tvar_t* type;
  static jl_tvar_t* julia_type() { return TypeVar<I>::tvar(); }
};

template<typename T>
struct julia_type_factory<BoxedValue<T>>
{
  static jl_datatype_t* julia_type() { return jl_any_type; }
};

/// Helper for Singleton types (Type{T} in Julia)
template<typename T>
struct SingletonType
{
};

template<typename T>
struct static_type_mapping<SingletonType<T>>
{
  using type = jl_datatype_t*;
};

template<typename T>
struct julia_type_factory<SingletonType<T>>
{
  static inline jl_datatype_t* julia_type()
  {
    return apply_type((jl_value_t *)jl_type_type, ::jlcxx::julia_base_type<T>());
  }
};

template<typename T>
struct ConvertToCpp<SingletonType<T>, NoMappingTrait>
{
  SingletonType<T> operator()(jl_datatype_t*) const
  {
    return SingletonType<T>();
  }
};

template<typename T>
struct ConvertToJulia<SingletonType<T>, NoMappingTrait>
{
  jl_datatype_t* operator()(SingletonType<T>) const
  {
    static jl_datatype_t* result = julia_base_type<T>();
    return result;
  }
};

/// Helper to encapsulate a strictly typed number type. Numbers typed like this will not be involved in the convenience-overloads that allow passing e.g. an Int to a Float64 argument
template<typename NumberT>
struct StrictlyTypedNumber
{
  NumberT value;
};

template<typename NumberT> struct static_type_mapping<StrictlyTypedNumber<NumberT>>
{
  typedef StrictlyTypedNumber<NumberT> type;
};

template<typename NumberT> struct static_type_mapping<StrictlyTypedNumber<NumberT>&>
{
  static_assert(sizeof(NumberT)==0, "References to StrictlyTypedNumber are not allowed, use values instead");
};
template<typename NumberT> struct static_type_mapping<const StrictlyTypedNumber<NumberT>&>
{
  static_assert(sizeof(NumberT)==0, "References to StrictlyTypedNumber are not allowed, use values instead");
};

template<typename NumberT> struct julia_type_factory<StrictlyTypedNumber<NumberT>>
{
  static jl_datatype_t* julia_type()
  {
    return apply_type(::jlcxx::julia_type("StrictlyTypedNumber"), ::jlcxx::julia_type<NumberT>());
  }
};

template<typename NumberT> struct IsMirroredType<std::complex<NumberT>> : std::true_type {};

template<typename NumberT> struct julia_type_factory<std::complex<NumberT>>
{
  static jl_datatype_t* julia_type()
  {
    return apply_type(jlcxx::julia_type("Complex"), ::jlcxx::julia_type<NumberT>());
  }
};

}

#endif
