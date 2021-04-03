#ifndef JLCXX_FUNCTIONS_HPP
#define JLCXX_FUNCTIONS_HPP

#include <sstream>
#include <vector>

#include "array.hpp"
#include "type_conversion.hpp"

// This header provides helper functions to call Julia functions from C++

namespace jlcxx
{

/// Wrap a Julia function for easy calling
class JLCXX_API JuliaFunction
{
public:
  /// Construct using a function name and module name. Searches the current module by default. Throws if the function was not found.
  JuliaFunction(const std::string& name, const std::string& module_name = "");
  /// Construct directly from a pointer (throws if pointer is null)
  JuliaFunction(jl_function_t* fpointer);

  /// Access to the raw pointer
  jl_function_t* pointer() const
  {
    return m_function;
  }

  /// Call a julia function, converting the arguments to the corresponding Julia types
  template<typename... ArgumentsT>
  jl_value_t* operator()(ArgumentsT&&... args) const;

private:
  struct StoreArgs
  {
    StoreArgs(jl_value_t** arg_array) : m_arg_array(arg_array)
    {
    }

    template<typename ArgT, typename... ArgsT>
    void push(ArgT&& a, ArgsT&&... args)
    {
      push(std::forward<ArgT>(a));
      push(std::forward<ArgsT>(args)...);
    }

    template<typename ArgT>
    void push(ArgT&& a)
    {
      m_arg_array[m_i++] = box<ArgT>(a);
    }

    void push(jl_value_t* a)
    {
      m_arg_array[m_i++] = a;
    }

    void push() {}

    jl_value_t** m_arg_array;
    int m_i = 0;
  };
  jl_function_t* m_function;
};

template<typename... ArgumentsT>
jl_value_t* JuliaFunction::operator()(ArgumentsT&&... args) const
{
  (create_if_not_exists<ArgumentsT>(), ...);

  const int nb_args = sizeof...(args);

  jl_value_t** julia_args;
  JL_GC_PUSHARGS(julia_args, nb_args+1); // The last element is the result

  // Process arguments
  StoreArgs store_args(julia_args);
  store_args.push(std::forward<ArgumentsT>(args)...);
  for(int i = 0; i != nb_args; ++i)
  {
    if(julia_args[i] == nullptr)
    {
      JL_GC_POP();
      std::stringstream sstr;
      sstr << "Unsupported Julia function argument type at position " << i;
      throw std::runtime_error(sstr.str());
    }
  }

  // Do the call
  julia_args[nb_args] = jl_call(m_function, julia_args, nb_args);
  if (jl_exception_occurred())
  {
    jl_call2(jl_get_function(jl_base_module, "show"), jl_stderr_obj(), jl_exception_occurred());
    jl_printf(jl_stderr_stream(), "\n");
    JL_GC_POP();
    return nullptr;
  }

  JL_GC_POP();
  return julia_args[nb_args];
}

/// Data corresponds to immutable with the same name on the Julia side
struct SafeCFunction
{
  void* fptr;
  jl_datatype_t* return_type;
  jl_array_t* argtypes;
};

// Direct conversion
template<> struct static_type_mapping<SafeCFunction>
{
  typedef SafeCFunction type;
};

template<> struct julia_type_factory<SafeCFunction>
{
  static jl_datatype_t* julia_type() { return (jl_datatype_t*)jlcxx::julia_type("SafeCFunction"); }
};

template<>
struct ConvertToCpp<SafeCFunction>
{
  SafeCFunction operator()(const SafeCFunction& julia_value) const
  {
    return julia_value;
  }

  SafeCFunction operator()(jl_value_t* julia_value) const
  {
    return *reinterpret_cast<SafeCFunction*>(jl_data_ptr(julia_value));
  }
};

namespace detail
{
  template<typename SignatureT>
  struct SplitSignature;

  template<typename R, typename... ArgsT>
  struct SplitSignature<R(ArgsT...)>
  {
    typedef R return_type;
    typedef R(*fptr_t)(ArgsT...);

    std::vector<jl_datatype_t*> operator()()
    {
      return std::vector<jl_datatype_t*>({julia_type<ArgsT>()...});
    }

    fptr_t cast_ptr(void* ptr)
    {
      return reinterpret_cast<fptr_t>(ptr);
    }
  };
}

/// Type-checking on return type and arguments of a cfunction (void* pointer)
template<typename SignatureT>
typename detail::SplitSignature<SignatureT>::fptr_t make_function_pointer(SafeCFunction data)
{
  typedef detail::SplitSignature<SignatureT> SplitterT;
  JL_GC_PUSH3(&data.fptr, &data.return_type, &data.argtypes);

  // Check return type
  jl_datatype_t* expected_rt = julia_type<typename SplitterT::return_type>();
  if(expected_rt != data.return_type)
  {
    JL_GC_POP();
    throw std::runtime_error("Incorrect datatype for cfunction return type, expected " + julia_type_name(expected_rt) + " but got " + julia_type_name(data.return_type));
  }

  // Check arguments
  const std::vector<jl_datatype_t*> expected_argstypes = SplitterT()();
  ArrayRef<jl_value_t*> argtypes(data.argtypes);
  const int nb_args = expected_argstypes.size();
  if(nb_args != static_cast<int>(argtypes.size()))
  {
    std::stringstream err_sstr;
    err_sstr << "Incorrect number of arguments for cfunction, expected: " << nb_args << ", obtained: " << argtypes.size();
    JL_GC_POP();
    throw std::runtime_error(err_sstr.str());
  }
  for(int i = 0; i != nb_args; ++i)
  {
    jl_datatype_t* argt = (jl_datatype_t*)argtypes[i];
    if(argt != expected_argstypes[i])
    {
      std::stringstream err_sstr;
      err_sstr << "Incorrect argument type for cfunction at position " << i+1 << ", expected: " << julia_type_name(expected_argstypes[i]) << ", obtained: " << julia_type_name(argt);
      JL_GC_POP();
      throw std::runtime_error(err_sstr.str());
    }
  }
  JL_GC_POP();
  return SplitterT().cast_ptr(data.fptr);
}

struct FunctionPtrTrait {};

template<typename R, typename...ArgsT>
struct MappingTrait<R(*)(ArgsT...)>
{
  using type = FunctionPtrTrait;
};

/// Implicit conversion to pointer type
template<typename R, typename...ArgsT> struct static_type_mapping<R(*)(ArgsT...)>
{
  typedef SafeCFunction type;
};

template<typename R, typename...ArgsT> struct julia_type_factory<R(*)(ArgsT...)>
{
  static jl_datatype_t* julia_type()
  {
    create_if_not_exists<R>();
    (create_if_not_exists<ArgsT>(), ...);
    return (jl_datatype_t*)jlcxx::julia_type("SafeCFunction");
  }
};

template<typename R, typename...ArgsT>
struct ConvertToCpp<R(*)(ArgsT...), FunctionPtrTrait>
{
  typedef R(*fptr_t)(ArgsT...);
  fptr_t operator()(const SafeCFunction& julia_value) const
  {
    return make_function_pointer<R(ArgsT...)>(julia_value);
  }
};

}

#endif
