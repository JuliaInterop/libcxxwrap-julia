#ifndef JLCXX_MODULE_HPP
#define JLCXX_MODULE_HPP

#include <cassert>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <sstream>
#include <typeinfo>
#include <vector>

#include "array.hpp"
#include "type_conversion.hpp"

namespace jlcxx
{

/// Wrappers for creating new datatype
JLCXX_API jl_datatype_t* new_datatype(jl_sym_t *name,
                            jl_module_t* module,
                            jl_datatype_t *super,
                            jl_svec_t *parameters,
                            jl_svec_t *fnames, jl_svec_t *ftypes,
                            int abstract, int mutabl,
                            int ninitialized);

JLCXX_API jl_datatype_t* new_bitstype(jl_sym_t *name,
                            jl_module_t* module,
                            jl_datatype_t *super,
                            jl_svec_t *parameters, const size_t nbits);

/// Some helper functions
namespace detail
{

// Need to treat void specially
template<typename R, typename... Args>
struct ReturnTypeAdapter
{
  using return_type = decltype(convert_to_julia(std::declval<R>()));

  inline return_type operator()(const void* functor, static_julia_type<Args>... args)
  {
    auto std_func = reinterpret_cast<const std::function<R(Args...)>*>(functor);
    assert(std_func != nullptr);
    return convert_to_julia((*std_func)(convert_to_cpp<Args>(args)...));
  }
};

template<typename... Args>
struct ReturnTypeAdapter<void, Args...>
{
  inline void operator()(const void* functor, static_julia_type<Args>... args)
  {
    auto std_func = reinterpret_cast<const std::function<void(Args...)>*>(functor);
    assert(std_func != nullptr);
    (*std_func)(convert_to_cpp<Args>(args)...);
  }
};

/// Call a C++ std::function, passed as a void pointer since it comes from Julia
template<typename R, typename... Args>
struct CallFunctor
{
  using return_type = decltype(ReturnTypeAdapter<R, Args...>()(std::declval<const void*>(), std::declval<static_julia_type<Args>>()...));

  static return_type apply(const void* functor, static_julia_type<Args>... args)
  {
    try
    {
      return ReturnTypeAdapter<R, Args...>()(functor, args...);
    }
    catch(const std::exception& err)
    {
      jl_error(err.what());
    }

    return return_type();
  }
};

/// Make a vector with the types in the variadic template parameter pack
template<typename... Args>
std::vector<jl_datatype_t*> argtype_vector()
{
  return {julia_type<Args>()...};
}

template<typename... Args>
struct NeedConvertHelper
{
  bool operator()()
  {
    for(const bool b : {std::is_same<static_julia_type<Args>,Args>::value...})
    {
      if(!b)
        return true;
    }
    return false;
  }
};

template<>
struct NeedConvertHelper<>
{
  bool operator()()
  {
    return false;
  }
};

} // end namespace detail

/// Convenience function to create an object with a finalizer attached
template<typename T, bool finalize=true, typename... ArgsT>
jl_value_t* create(ArgsT&&... args)
{
  jl_datatype_t* dt = julia_type<T>();
  assert(jl_is_mutable_datatype(dt));
  

  T* cpp_obj = new T(std::forward<ArgsT>(args)...);

  return boxed_cpp_pointer(cpp_obj, dt, finalize);
}

/// Safe downcast to base type
template<typename T>
struct DownCast
{
  static inline supertype<T>& apply(T& base)
  {
    return static_cast<supertype<T>&>(base);
  }
};

// The CxxWrap Julia module
extern jl_module_t* g_cxxwrap_module;
extern jl_datatype_t* g_cppfunctioninfo_type;

typedef void (*protect_f_t)(jl_value_t*);
extern protect_f_t g_protect_from_gc;
extern protect_f_t g_unprotect_from_gc;

class JLCXX_API Module;

/// Abstract base class for storing any function
class JLCXX_API FunctionWrapperBase
{
public:
  FunctionWrapperBase(Module* mod, jl_datatype_t* return_type) : m_module(mod), m_return_type(return_type)
  {
  }

  /// Types of the arguments (used in the wrapper signature)
  virtual std::vector<jl_datatype_t*> argument_types() const = 0;

  /// Return type
  jl_datatype_t* return_type() const { return m_return_type; }

  void set_return_type(jl_datatype_t* dt) { m_return_type = dt; }

  virtual ~FunctionWrapperBase() {}

  inline void set_name(jl_value_t* name)
  {
    protect_from_gc(name);
    m_name = name;
  }

  inline jl_value_t* name() const
  {
    return m_name;
  }

  inline int_t pointer_index() { return m_pointer_index; }
  inline int_t thunk_index() { return m_thunk_index; }

  inline void set_override_module(jl_module_t* mod) { m_override_module = (jl_value_t*)mod; }
  inline jl_value_t* override_module() const { return m_override_module; }

protected:
  /// Function pointer as void*, since that's what Julia expects
  virtual void* pointer() = 0;

  /// The thunk (i.e. std::function) to pass as first argument to the function pointed to by function_pointer
  virtual void* thunk() = 0;

  void set_pointer_indices();
private:
  jl_value_t* m_name;
  Module* m_module;
  jl_datatype_t* m_return_type = nullptr;

  int_t m_pointer_index = 0;
  int_t m_thunk_index = 0;

  // The module in which the function is overridden, e.g. jl_base_module when trying to override Base.getindex.
  jl_value_t* m_override_module = jl_nothing;
};

/// Implementation of function storage, case of std::function
template<typename R, typename... Args>
class FunctionWrapper : public FunctionWrapperBase
{
public:
  typedef std::function<R(Args...)> functor_t;

  FunctionWrapper(Module* mod, const functor_t &function) : FunctionWrapperBase(mod, julia_return_type<R>()), m_function(function)
  {
    set_pointer_indices();
  }

  virtual std::vector<jl_datatype_t*> argument_types() const
  {
    return detail::argtype_vector<Args...>();
  }

protected:
  virtual void* pointer()
  {
    return reinterpret_cast<void*>(detail::CallFunctor<R, Args...>::apply);
  }

  virtual void* thunk()
  {
    return reinterpret_cast<void*>(&m_function);
  }

private:
  functor_t m_function;
};

/// Implementation of function storage, case of a function pointer
template<typename R, typename... Args>
class FunctionPtrWrapper : public FunctionWrapperBase
{
public:
  typedef std::function<R(Args...)> functor_t;

  FunctionPtrWrapper(Module* mod, R (*f)(Args...)) : FunctionWrapperBase(mod, julia_return_type<R>()), m_function(f)
  {
    set_pointer_indices();
  }

  virtual std::vector<jl_datatype_t*> argument_types() const
  {
    return detail::argtype_vector<Args...>();
  }

protected:
  virtual void* pointer()
  {
    return reinterpret_cast<void*>(m_function);
  }

  virtual void* thunk()
  {
    return nullptr;
  }

private:
  R(*m_function)(Args...);
};

/// Indicate that a parametric type is to be added
template<typename... ParametersT>
struct Parametric
{
};

template<typename T>
class TypeWrapper;

/// Specialise this to instantiate parametric types when first used in a wrapper
template<typename T>
struct InstantiateParametricType
{
  // Returns int to expand parameter packs into an initialization list
  int operator()(Module&) const
  {
    return 0;
  }
};

template<typename... TypesT>
void instantiate_parametric_types(Module& m)
{
  auto unused = {InstantiateParametricType<remove_const_ref<TypesT>>()(m)...};
}

namespace detail
{

template<typename T>
struct GetJlType
{
  jl_datatype_t* operator()() const
  {
    try
    {
      return julia_base_type<remove_const_ref<T>>();
    }
    catch(...)
    {
      // The assumption here is that unmapped types are not needed, i.e. in default argument lists
      return nullptr;
    }
  }
};

template<int I>
struct GetJlType<TypeVar<I>>
{
  jl_tvar_t* operator()() const
  {
    return TypeVar<I>::tvar();
  }
};

template<typename T, T Val>
struct GetJlType<std::integral_constant<T, Val>>
{
  jl_value_t* operator()() const
   {
    return box(convert_to_julia(Val));
  }
};

template<typename T>
struct IsParametric
{
  static constexpr bool value = false;
};

template<template<typename...> class T, int I, typename... ParametersT>
struct IsParametric<T<TypeVar<I>, ParametersT...>>
{
  static constexpr bool value = true;
};

template<typename... ArgsT>
inline jl_value_t* make_fname(const std::string& nametype, ArgsT... args)
{
  jl_value_t* name = nullptr;
  JL_GC_PUSH1(&name);
  name = jl_new_struct((jl_datatype_t*)julia_type(nametype), args...);
  protect_from_gc(name);
  JL_GC_POP();

  return name;
}

} // namespace detail

// Encapsulate a list of parameters, using types only
template<typename... ParametersT>
struct ParameterList
{
  static constexpr int nb_parameters = sizeof...(ParametersT);

  jl_svec_t* operator()(const int n = nb_parameters)
  {
    jl_svec_t* result = jl_svec(n, detail::GetJlType<ParametersT>()()...);
    for(int i = 0; i != n; ++i)
    {
      if(jl_svecref(result,i) == nullptr)
      {
        std::vector<std::string> typenames({(typeid(ParametersT).name())...}); 
        throw std::runtime_error("Attempt to use unmapped type " + typenames[i] + " in parameter list");
      }
    }
    return result;
  }
};

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

  template<typename T1, typename T2>
  struct CombineParameterLists
  {
  };

  template<typename... Params1, typename... Params2>
  struct CombineParameterLists<ParameterList<Params1...>, ParameterList<Params2...>>
  {
    using type = ParameterList<Params1..., Params2...>;
  };
}

template<typename T> using remove_duplicates = typename detail::RemoveDuplicates<T>::type;
template<typename T1, typename T2> using combine_parameterlists = typename detail::CombineParameterLists<T1,T2>::type;

using fundamental_int_types = remove_duplicates<ParameterList
<
  short int,
  unsigned short int,
  int,
  unsigned int,
  long long int,
  unsigned long long int
>>;

using fixed_int_types = remove_duplicates<ParameterList
<
  int8_t,uint8_t,
  int16_t,uint16_t,
  int32_t,uint32_t,
  int64_t,uint64_t
>>;

/// Store all exposed C++ functions associated with a module
class JLCXX_API Module
{
public:

  Module(jl_module_t* jl_mod);

  void append_function(FunctionWrapperBase* f)
  {
    assert(f != nullptr);
    m_functions.push_back(std::shared_ptr<FunctionWrapperBase>(f));
    assert(m_functions.back() != nullptr);
  }

  /// Define a new function
  template<typename R, typename... Args>
  FunctionWrapperBase& method(const std::string& name,  std::function<R(Args...)> f)
  {
    instantiate_parametric_types<R, Args...>(*this);
    auto* new_wrapper = new FunctionWrapper<R, Args...>(this, f);
    new_wrapper->set_name((jl_value_t*)jl_symbol(name.c_str()));
    append_function(new_wrapper);
    return *new_wrapper;
  }

  /// Define a new function. Overload for pointers
  template<typename R, typename... Args>
  FunctionWrapperBase& method(const std::string& name,  R(*f)(Args...), const bool force_convert = false)
  {
    const bool need_convert = force_convert || detail::NeedConvertHelper<R, Args...>()();

    // Conversion is automatic when using the std::function calling method, so if we need conversion we use that
    if(need_convert)
    {
      return method(name, std::function<R(Args...)>(f));
    }

    instantiate_parametric_types<R, Args...>(*this);

    // No conversion needed -> call can be through a naked function pointer
    auto* new_wrapper = new FunctionPtrWrapper<R, Args...>(this, f);
    new_wrapper->set_name((jl_value_t*)jl_symbol(name.c_str()));
    append_function(new_wrapper);
    return *new_wrapper;
  }

  /// Define a new function. Overload for lambda
  template<typename LambdaT>
  FunctionWrapperBase& method(const std::string& name, LambdaT&& lambda, typename std::enable_if<!std::is_member_function_pointer<LambdaT>::value, bool>::type = true)
  {
    return add_lambda(name, std::forward<LambdaT>(lambda), &LambdaT::operator());
  }

  /// Add a constructor with the given argument types for the given datatype (used to get the name)
  template<typename T, typename... ArgsT>
  void constructor(jl_datatype_t* dt, bool finalize=true)
  {
    FunctionWrapperBase &new_wrapper = finalize ? method("dummy", [](ArgsT... args) { return create<T, true>(args...); }) : method("dummy", [](ArgsT... args) { return create<T, false>(args...); });
    new_wrapper.set_name(detail::make_fname("ConstructorFname", dt));
  }

  /// Loop over the functions
  template<typename F>
  void for_each_function(const F f) const
  {
    auto funcs_copy = m_functions;
    for(const auto &item : funcs_copy)
    {
      assert(item != nullptr);
      f(*item);
    }
    // Account for any new functions added during the loop
    while(funcs_copy.size() != m_functions.size())
    {
      const std::size_t oldsize = funcs_copy.size();
      const std::size_t newsize = m_functions.size();
      funcs_copy = m_functions;
      for(std::size_t i = oldsize; i != newsize; ++i)
      {
        assert(funcs_copy[i] != nullptr);
        f(*funcs_copy[i]);
      }
    }
  }

  inline jlcxx::FunctionWrapperBase& last_function()
  {
    return *m_functions.back();
  }

  /// Add a composite type
  template<typename T, typename SuperParametersT=ParameterList<>, typename JLSuperT=jl_datatype_t>
  TypeWrapper<T> add_type(const std::string& name, JLSuperT* super = jl_any_type);

  /// Add types that are directly mapped to a Julia struct
  template<typename T>
  void map_type(const std::string& name)
  {
    set_julia_type<T>((jl_datatype_t*)julia_type(name, m_jl_mod));
  }

  template<typename T, typename JLSuperT=jl_datatype_t>
  void add_bits(const std::string& name, JLSuperT* super = jl_any_type);

  /// Set a global constant value at the module level
  template<typename T>
  void set_const(const std::string& name, T&& value)
  {
    if(m_jl_constants.count(name) != 0)
    {
      throw std::runtime_error("Duplicate registration of constant " + name);
    }
    jl_value_t* boxed_const = box(std::forward<T>(value));
    protect_from_gc(boxed_const);
    m_jl_constants[name] = boxed_const;
  }

  std::string name() const
  {
    return module_name(m_jl_mod);
  }

  void bind_constants(jl_module_t* mod);

  jl_datatype_t* get_julia_type(const char* name)
  {
    if(m_jl_constants.count(name) != 0 && jl_is_datatype(m_jl_constants[name]))
    {
      return (jl_datatype_t*)m_jl_constants[name];
    }

    return nullptr;
  }

  void register_type(jl_datatype_t* box_type)
  {
    m_box_types.push_back(box_type);
  }

  const std::vector<jl_datatype_t*> box_types() const
  {
    return m_box_types;
  }

  jl_module_t* julia_module() const
  {
    return m_jl_mod;
  }

  int_t store_pointer(void* ptr);

private:

  template<typename T>
  void add_default_constructor(std::true_type, jl_datatype_t* dt);

  template<typename T>
  void add_default_constructor(std::false_type, jl_datatype_t*)
  {
  }

  template<typename T>
  void add_copy_constructor(std::true_type, jl_datatype_t*)
  {
    method("deepcopy_internal", [this](const T& other, ObjectIdDict)
    {
      return create<T>(other);
    });
  }

  template<typename T>
  void add_copy_constructor(std::false_type, jl_datatype_t*)
  {
  }

  template<typename T, typename SuperParametersT, typename JLSuperT>
  TypeWrapper<T> add_type_internal(const std::string& name, JLSuperT* super);

  template<typename R, typename LambdaT, typename... ArgsT>
  FunctionWrapperBase& add_lambda(const std::string& name, LambdaT&& lambda, R(LambdaT::*)(ArgsT...) const)
  {
    return method(name, std::function<R(ArgsT...)>(std::forward<LambdaT>(lambda)));
  }

  jl_module_t* m_jl_mod;
  ArrayRef<void*> m_pointer_array;
  std::vector<std::shared_ptr<FunctionWrapperBase>> m_functions;
  std::map<std::string, jl_value_t*> m_jl_constants;
  std::vector<jl_datatype_t*> m_box_types;

  template<class T> friend class TypeWrapper;
};

template<typename T>
void Module::add_default_constructor(std::true_type, jl_datatype_t* dt)
{
  this->constructor<T>(dt);
}

// Specialize this to build the correct parameter list, wrapping non-types in integral constants
// There is no way to provide a template here that matches all possible combinations of type and non-type arguments
template<typename T>
struct BuildParameterList
{
  typedef ParameterList<> type;
};

template<typename T> using parameter_list = typename BuildParameterList<T>::type;

// Match any combination of types only
template<template<typename...> class T, typename... ParametersT>
struct BuildParameterList<T<ParametersT...>>
{
  typedef ParameterList<ParametersT...> type;
};

// Match any number of int parameters
template<template<int...> class T, int... ParametersT>
struct BuildParameterList<T<ParametersT...>>
{
  typedef ParameterList<std::integral_constant<int, ParametersT>...> type;
};

namespace detail
{
  template<typename... Types>
  struct DoApply;

  template<>
  struct DoApply<>
  {
    template<typename WrapperT, typename FunctorT>
    void operator()(WrapperT&, FunctorT&&)
    {
    }
  };

  template<typename AppT>
  struct DoApply<AppT>
  {
    template<typename WrapperT, typename FunctorT>
    void operator()(WrapperT& w, FunctorT&& ftor)
    {
      w.template apply<AppT>(std::forward<FunctorT>(ftor));
    }
  };

  template<typename... Types>
  struct DoApply<ParameterList<Types...>>
  {
    template<typename WrapperT, typename FunctorT>
    void operator()(WrapperT& w, FunctorT&& ftor)
    {
      DoApply<Types...>()(w, std::forward<FunctorT>(ftor));
    }
  };

  template<typename T1, typename... Types>
  struct DoApply<T1, Types...>
  {
    template<typename WrapperT, typename FunctorT>
    void operator()(WrapperT& w, FunctorT&& ftor)
    {
      DoApply<T1>()(w, std::forward<FunctorT>(ftor));
      DoApply<Types...>()(w, std::forward<FunctorT>(ftor));
    }
  };
}

/// Execute a functor on each type
template<typename... Types>
struct ForEachType;

template<>
struct ForEachType<>
{
  template<typename FunctorT>
  void operator()(FunctorT&&)
  {
  }
};

template<typename AppT>
struct ForEachType<AppT>
{
  template<typename FunctorT>
  void operator()(FunctorT&& ftor)
  {
#ifdef _MSC_VER
    ftor.operator()<AppT>();
#else 
    ftor.template operator()<AppT>();
#endif
  }
};

template<typename... Types>
struct ForEachType<ParameterList<Types...>>
{
  template<typename FunctorT>
  void operator()(FunctorT&& ftor)
  {
    ForEachType<Types...>()(std::forward<FunctorT>(ftor));
  }
};

template<typename T1, typename... Types>
struct ForEachType<T1, Types...>
{
  template<typename FunctorT>
  void operator()(FunctorT&& ftor)
  {
    ForEachType<T1>()(std::forward<FunctorT>(ftor));
    ForEachType<Types...>()(std::forward<FunctorT>(ftor));
  }
};

template<typename T, typename FunctorT>
void for_each_type(FunctorT&& f)
{
  ForEachType<T>()(f);
}

/// Trait to allow user-controlled disabling of the default constructor
template<typename T>
struct DefaultConstructible : std::is_default_constructible<T>
{
};

/// Trait to allow user-controlled disabling of the copy constructor
template<typename T>
struct CopyConstructible : std::is_copy_constructible<T>
{
};

template<typename... Types>
struct UnpackedTypeList
{
};

template<typename ApplyT, typename... TypeLists>
struct CombineTypes;

template<typename ApplyT, typename... UnpackedTypes>
struct CombineTypes<ApplyT, UnpackedTypeList<UnpackedTypes...>>
{
  typedef typename ApplyT::template apply<UnpackedTypes...> type;
};

template<typename ApplyT, typename... UnpackedTypes, typename... Types, typename... OtherTypeLists>
struct CombineTypes<ApplyT, UnpackedTypeList<UnpackedTypes...>, ParameterList<Types...>, OtherTypeLists...>
{
  typedef CombineTypes<ApplyT, UnpackedTypeList<UnpackedTypes...>, ParameterList<Types...>, OtherTypeLists...> ThisT;

  template<typename T1>
  struct type_unpack
  {
    typedef UnpackedTypeList<UnpackedTypes..., T1> unpacked_t;
    typedef CombineTypes<ApplyT, unpacked_t, OtherTypeLists...> combined_t;
  };
  
  typedef ParameterList<typename ThisT::template type_unpack<Types>::combined_t::type...> type;
};

template<typename ApplyT, typename... Types, typename... OtherTypeLists>
struct CombineTypes<ApplyT, ParameterList<Types...>, OtherTypeLists...>
{
  typedef CombineTypes<ApplyT, ParameterList<Types...>, OtherTypeLists...> ThisT;

  template<typename T1>
  struct type_unpack
  {
    typedef UnpackedTypeList<T1> unpacked_t;
    typedef CombineTypes<ApplyT, unpacked_t, OtherTypeLists...> combined_t;
  };

  typedef ParameterList<typename ThisT::template type_unpack<Types>::combined_t::type...> type;
};

// Default ApplyT implementation
template<template<typename...> class TemplateT>
struct ApplyType
{
  template<typename... Types> using apply = TemplateT<Types...>;
};

/// Helper class to wrap type methods
template<typename T>
class TypeWrapper
{
public:
  typedef T type;

  TypeWrapper(Module& mod, jl_datatype_t* dt, jl_datatype_t* box_dt) :
    m_module(mod),
    m_dt(dt),
    m_box_dt(box_dt)
  {
  }

  TypeWrapper(Module& mod, TypeWrapper<T>& other) :
    m_module(mod),
    m_dt(other.m_dt),
    m_box_dt(other.m_box_dt)
  {
  }

  /// Add a constructor with the given argument types
  template<typename... ArgsT>
  TypeWrapper<T>& constructor(bool finalize=true)
  {
    m_module.constructor<T, ArgsT...>(m_dt, finalize);
    return *this;
  }

  /// Define a member function
  template<typename R, typename CT, typename... ArgsT>
  TypeWrapper<T>& method(const std::string& name, R(CT::*f)(ArgsT...))
  {
    m_module.method(name, [f](T& obj, ArgsT... args) -> R { return (obj.*f)(args...); } );
    return *this;
  }

  /// Define a member function, const version
  template<typename R, typename CT, typename... ArgsT>
  TypeWrapper<T>& method(const std::string& name, R(CT::*f)(ArgsT...) const)
  {
    m_module.method(name, [f](const T& obj, ArgsT... args) -> R { return (obj.*f)(args...); } );
    return *this;
  }

  /// Define a "member" function using a lambda
  template<typename LambdaT>
  TypeWrapper<T>& method(const std::string& name, LambdaT&& lambda, typename std::enable_if<!std::is_member_function_pointer<LambdaT>::value, bool>::type = true)
  {
    m_module.method(name, std::forward<LambdaT>(lambda));
    return *this;
  }

  /// Call operator overload. For concrete type box to work around https://github.com/JuliaLang/julia/issues/14919
  template<typename R, typename CT, typename... ArgsT>
  TypeWrapper<T>& method(R(CT::*f)(ArgsT...))
  {
    m_module.method("operator()", [f](T& obj, ArgsT... args) -> R { return (obj.*f)(args...); } )
      .set_name(detail::make_fname("CallOpOverload", m_box_dt));
    return *this;
  }
  template<typename R, typename CT, typename... ArgsT>
  TypeWrapper<T>& method(R(CT::*f)(ArgsT...) const)
  {
    m_module.method("operator()", [f](const T& obj, ArgsT... args) -> R { return (obj.*f)(args...); } )
      .set_name(detail::make_fname("CallOpOverload", m_box_dt));
    return *this;
  }

  /// Overload operator() using a lambda
  template<typename LambdaT>
  TypeWrapper<T>& method(LambdaT&& lambda)
  {
    m_module.method("operator()", std::forward<LambdaT>(lambda))
      .set_name(detail::make_fname("CallOpOverload", m_box_dt));
    return *this;
  }

  template<typename... AppliedTypesT, typename FunctorT>
  TypeWrapper<T>& apply(FunctorT&& apply_ftor)
  {
    static_assert(detail::IsParametric<T>::value, "Apply can only be called on parametric types");
    auto dummy = {this->template apply_internal<AppliedTypesT>(std::forward<FunctorT>(apply_ftor))...};
    return *this;
  }

  /// Apply all possible combinations of the given types (see example)
  template<template<typename...> class TemplateT, typename... TypeLists, typename FunctorT>
  void apply_combination(FunctorT&& ftor);

  template<typename ApplyT, typename... TypeLists, typename FunctorT>
  void apply_combination(FunctorT&& ftor);

  // Access to the module
  Module& module()
  {
    return m_module;
  }

  jl_datatype_t* dt()
  {
    return m_dt;
  }

private:

  template<typename AppliedT, typename FunctorT>
  int apply_internal(FunctorT&& apply_ftor)
  {
    static_assert(parameter_list<AppliedT>::nb_parameters != 0, "No parameters found when applying type. Specialize jlcxx::BuildParameterList for your combination of type and non-type parameters.");
    static_assert(parameter_list<AppliedT>::nb_parameters >= parameter_list<T>::nb_parameters, "Parametric type applied to wrong number of parameters.");
    const bool is_abstract = jl_is_abstracttype(m_dt);

    jl_datatype_t* app_dt = (jl_datatype_t*)apply_type((jl_value_t*)m_dt, parameter_list<AppliedT>()(parameter_list<T>::nb_parameters));
    jl_datatype_t* app_box_dt = (jl_datatype_t*)apply_type((jl_value_t*)m_box_dt, parameter_list<AppliedT>()(parameter_list<T>::nb_parameters));

    set_julia_type<AppliedT>(app_box_dt);
    m_module.add_default_constructor<AppliedT>(DefaultConstructible<AppliedT>(), app_dt);
    m_module.add_copy_constructor<AppliedT>(CopyConstructible<AppliedT>(), app_dt);

    apply_ftor(TypeWrapper<AppliedT>(m_module, app_dt, app_box_dt));

    m_module.register_type(app_box_dt);

    if(!std::is_same<supertype<AppliedT>,AppliedT>::value)
    {
      m_module.method("cxxdowncast", DownCast<AppliedT>::apply);
    }

    return 0;
  }
  Module& m_module;
  jl_datatype_t* m_dt;
  jl_datatype_t* m_box_dt;
};

template<typename ApplyT, typename... TypeLists> using combine_types = typename CombineTypes<ApplyT, TypeLists...>::type;

template<typename T>
template<template<typename...> class TemplateT, typename... TypeLists, typename FunctorT>
void TypeWrapper<T>::apply_combination(FunctorT&& ftor)
{
  this->template apply_combination<ApplyType<TemplateT>, TypeLists...>(std::forward<FunctorT>(ftor));
}

template<typename T>
template<typename ApplyT, typename... TypeLists, typename FunctorT>
void TypeWrapper<T>::apply_combination(FunctorT&& ftor)
{
  typedef typename CombineTypes<ApplyT, TypeLists...>::type applied_list;
  detail::DoApply<applied_list>()(*this, std::forward<FunctorT>(ftor));
}

template<typename T, typename SuperParametersT, typename JLSuperT>
TypeWrapper<T> Module::add_type_internal(const std::string& name, JLSuperT* super_generic)
{
  static constexpr bool is_parametric = detail::IsParametric<T>::value;
  static_assert(!IsImmutable<T>::value, "Immutable types (marked with IsImmutable) can't be added using add_type, map them directly to a struct instead and use map_type");
  static_assert(!std::is_scalar<T>::value, "Scalar types must be added using add_bits");

  if(m_jl_constants.count(name) > 0)
  {
    throw std::runtime_error("Duplicate registration of type or constant " + name);
  }

  jl_datatype_t* super = nullptr;

  jl_svec_t* parameters = nullptr;
  jl_svec_t* super_parameters = nullptr;
  jl_svec_t* fnames = nullptr;
  jl_svec_t* ftypes = nullptr;
  JL_GC_PUSH5(&super, &parameters, &super_parameters, &fnames, &ftypes);

  parameters = is_parametric ? parameter_list<T>()() : jl_emptysvec;
  fnames = jl_svec1(jl_symbol("cpp_object"));
  ftypes = jl_svec1(jl_voidpointer_type);

  if(jl_is_datatype(super_generic) && jl_nparams((jl_datatype_t*)super_generic) == 0)
  {
    super = (jl_datatype_t*)super_generic;
  }
  else
  {
    super_parameters = SuperParametersT::nb_parameters == 0 ? parameter_list<T>()() : SuperParametersT()();
    super = (jl_datatype_t*)apply_type((jl_value_t*)super_generic, super_parameters);
  }

  const std::string allocname = name+"Allocated";

  // Create the datatypes
  jl_datatype_t* base_dt = new_datatype(jl_symbol(name.c_str()), m_jl_mod, super, parameters, jl_emptysvec, jl_emptysvec, 1, 0, 0);
  protect_from_gc(base_dt);

  super = is_parametric ? (jl_datatype_t*)apply_type((jl_value_t*)base_dt, parameters) : base_dt;

  jl_datatype_t* box_dt = new_datatype(jl_symbol(allocname.c_str()), m_jl_mod, super, parameters, fnames, ftypes, 0, 1, 1);
  protect_from_gc(box_dt);

  // Register the type
  if(!is_parametric)
  {
    set_julia_type<T>(box_dt);
    add_default_constructor<T>(DefaultConstructible<T>(), base_dt);
    add_copy_constructor<T>(CopyConstructible<T>(), base_dt);
  }

  m_jl_constants[name] = is_parametric ? base_dt->name->wrapper : (jl_value_t*)base_dt;
  m_jl_constants[allocname] = is_parametric ? box_dt->name->wrapper : (jl_value_t*)box_dt;

  if(!is_parametric)
  {
    this->register_type(box_dt);
  }

  if(!is_parametric && !std::is_same<supertype<T>,T>::value)
  {
    method("cxxdowncast", DownCast<T>::apply);
  }

  JL_GC_POP();
  return TypeWrapper<T>(*this, base_dt, box_dt);
}

/// Add a composite type
template<typename T, typename SuperParametersT, typename JLSuperT>
TypeWrapper<T> Module::add_type(const std::string& name, JLSuperT* super)
{
  return add_type_internal<T, SuperParametersT>(name, super);
}

namespace detail
{
  template<typename T, bool>
  struct dispatch_set_julia_type;

  // non-parametric
  template<typename T>
  struct dispatch_set_julia_type<T, false>
  {
    void operator()(jl_datatype_t* dt)
    {
      set_julia_type<T>(dt);
    }
  };

  // parametric
  template<typename T>
  struct dispatch_set_julia_type<T, true>
  {
    void operator()(jl_datatype_t*)
    {
    }
  };
}

/// Add a bits type
template<typename T, typename JLSuperT>
void Module::add_bits(const std::string& name, JLSuperT* super)
{
  assert(jl_is_datatype(super));
  static constexpr bool is_parametric = detail::IsParametric<T>::value;
  static_assert(std::is_scalar<T>::value, "Bits types must be a scalar type");
  jl_svec_t* params = is_parametric ? parameter_list<T>()() : jl_emptysvec;
  JL_GC_PUSH1(&params);
  jl_datatype_t* dt = new_bitstype(jl_symbol(name.c_str()), m_jl_mod, (jl_datatype_t*)super, params, 8*sizeof(T));
  protect_from_gc(dt);
  JL_GC_POP();
  detail::dispatch_set_julia_type<T, is_parametric>()(dt);
  m_jl_constants[name] = (jl_value_t*)dt;
}

/// Registry containing different modules
class JLCXX_API ModuleRegistry
{
public:
  /// Create a module and register it
  Module& create_module(jl_module_t* jmod);

  Module& get_module(jl_module_t* mod) const
  {
    const auto iter = m_modules.find(mod);
    if(iter == m_modules.end())
    {
      throw std::runtime_error("Module with name " + module_name(mod) + " was not found in registry");
    }

    return *(iter->second);
  }

  bool has_module(jl_module_t* jmod) const
  {
    return m_modules.find(jmod) != m_modules.end();
  }

  bool has_current_module() { return m_current_module != nullptr; }
  Module& current_module();
  void reset_current_module() { m_current_module = nullptr; }

private:
  std::map<jl_module_t*, std::shared_ptr<Module>> m_modules;
  Module* m_current_module = nullptr;
};

JLCXX_API ModuleRegistry& registry();

/// Registry for functions that are called when the CxxWrap module is initialized
class InitHooks
{
public:
  typedef std::function<void()> hook_t;

  // Singleton implementation
  static InitHooks& instance();

  // add a new hook
  void add_hook(const hook_t hook);

  // run all hooks
  void run_hooks();
private:
  InitHooks();
  std::vector<hook_t> m_hooks;
};

/// Helper to register a hook on library load
struct RegisterHook
{
  template<typename F>
  RegisterHook(F&& f)
  {
    InitHooks::instance().add_hook(InitHooks::hook_t(f));
  }
};

JLCXX_API void register_core_types();

} // namespace jlcxx

/// Register a new module
extern "C" JLCXX_API void register_julia_module(jl_module_t* mod, void (*regfunc)(jlcxx::Module&));

#define JLCXX_MODULE extern "C" JLCXX_ONLY_EXPORTS void

#endif
