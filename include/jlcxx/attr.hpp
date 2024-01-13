#ifndef JLCXX_ATTR_HPP
#define JLCXX_ATTR_HPP

#include <string>
#include <vector>
#include <type_traits>
#include <functional>

#include "jlcxx_config.hpp"
#include "type_conversion.hpp"


// This header provides internal helper functionality for providing additional information like argument names and default arguments for C++ functions (method in module.hpp)

namespace jlcxx
{

namespace detail
{
  /// Helper type for function arguments
  template <bool IsKwArg>
  struct JLCXX_API BasicArg
  {
    static constexpr bool isKeywordArgument = IsKwArg;

    const char *name = nullptr;
    jl_value_t* defaultValue = nullptr;

    BasicArg(const char *name_) : name(name_) {}

    template <typename T>
    inline BasicArg &operator=(T value)
    {
      defaultValue = box<T>(std::forward<T>(value));
      return *this;
    }
  };
}

/// use jlcxx::arg("argumentName") to add function argument names, and jlcxx::arg("name")=value to define an argument with a default value
using arg = detail::BasicArg<false>;

///! use jlcxx::kwarg("argumentName") to define a keyword argument and with jlcxx::kwarg("name")=value you can add a default value for the argument
using kwarg = detail::BasicArg<true>;

/// enum for the force_convert parameter for raw function pointers
enum class calling_policy : bool
{
  ccall = false,
  std_function = true
};
/// default value for the calling_policy argument for Module::method with raw C++ function pointers
constexpr auto default_calling_policy = calling_policy::ccall;

/// enum for finalize parameter for constructors
enum class finalize_policy : bool
{
    no = false,
    yes = true
};
/// default value for the finalize_policy argument for Module::constructor
constexpr auto default_finalize_policy = finalize_policy::yes;


namespace detail
{
  /// SFINEA for argument processing, inspired/copied from pybind11 code (pybind11/attr.h)
  template<typename T, typename SFINEA = void>
  struct process_attribute;

  /// helper type for parsing argument and docstrings
  struct ExtraFunctionData
  {
    std::vector<arg> positionalArguments;
    std::vector<kwarg> keywordArguments;
    std::string doc;
    calling_policy force_convert = default_calling_policy;
    finalize_policy finalize = default_finalize_policy;

  };

  /// process docstring
  template<>
  struct process_attribute<const char*>
  {
    static inline void init(const char* s, ExtraFunctionData& f)
    {
      f.doc = s;
    }
  };

  template<>
  struct process_attribute<char*> : public process_attribute<const char*> {};

  /// process positional argument
  template<>
  struct process_attribute<arg>
  {
    static inline void init(arg&& a, ExtraFunctionData& f)
    {
      f.positionalArguments.emplace_back(std::move(a));
    }
  };

  /// process keyword argument
  template<>
  struct process_attribute<kwarg>
  {
    static inline void init(kwarg&& a, ExtraFunctionData& f)
    {
      f.keywordArguments.emplace_back(std::move(a));
    }
  };

  /// process calling_policy argument
  template<>
  struct process_attribute<calling_policy>
  {
    static inline void init(calling_policy force_convert, ExtraFunctionData& f)
    {
      f.force_convert = force_convert;
    }
  };

  /// process finalize_policy argument
  template<>
  struct process_attribute<finalize_policy>
  {
    static inline void init(finalize_policy finalize, ExtraFunctionData& f)
    {
        f.finalize = finalize;
    }
  };

  template<typename T>
  void parse_attributes_helper(ExtraFunctionData& f, T argi)
  {
    using T_ = typename std::decay_t<T>;
    process_attribute<T>::init(std::forward<T_>(argi), f);
  }

  /// initialize ExtraFunctionData from argument list
  template<bool AllowCallingPolicy = false, bool AllowFinalizePolicy = false, typename... Extra>
  ExtraFunctionData parse_attributes(Extra... extra)
  {
    // check that the calling_policy is only set if explicitly allowed
    constexpr bool contains_calling_policy = (std::is_same_v<calling_policy, Extra> || ... );
    static_assert( (!contains_calling_policy) || AllowCallingPolicy, "calling_policy can only be set for raw function pointers!");

    // chat that the finalize_policy is only set if explicitly allowed
    constexpr bool contains_finalize_policy = (std::is_same_v<finalize_policy, Extra> || ... );
    static_assert( (!contains_finalize_policy) || AllowFinalizePolicy, "finalize_policy can only be set for constructors!");

    ExtraFunctionData result;

    (parse_attributes_helper(result, std::move(extra)), ...);

    return result;
  }

  /// count occurences of specific type in parameter pack
  template<typename T, typename... Extra>
  constexpr int count_attributes()
  {
    return (0 + ... + int(std::is_same_v<T,Extra>));
  }

  static_assert(count_attributes<float, int, float, int, double>() == 1);
  static_assert(count_attributes<int, int, float, int, double, int, int>() == 4);

  /// check number of arguments matches annotated arguments if annotations for keyword arguments are present
  template<typename...  Extra>
  constexpr bool check_extra_argument_count(int n_arg)
  {
    // with keyword arguments, the number of annotated arguments must match the number of actual arguments
    constexpr auto n_extra_arg = count_attributes<arg, Extra...>();
    constexpr auto n_extra_kwarg = count_attributes<kwarg, Extra...>();
    return n_extra_kwarg == 0 || n_arg == n_extra_arg + n_extra_kwarg;
  }

  /// simple helper for checking if a template argument has a call operator (e.g. is a lambda)
  template<class T, typename SFINEA = void>
  struct has_call_operator : std::false_type {};

  template<class T>
  struct has_call_operator<T, std::void_t<decltype(&T::operator())>> : std::true_type {};

  static_assert(!has_call_operator<const char*>::value);
  static_assert(has_call_operator<std::function<void()>>::value);
}

}

#endif
