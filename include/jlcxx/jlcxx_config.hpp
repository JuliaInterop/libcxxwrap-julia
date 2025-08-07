#ifndef JLCXX_CONFIG_HPP
#define JLCXX_CONFIG_HPP

#ifdef _WIN32
  #define JLCXX_USE_TYPE_MAP
  #ifdef JLCXX_EXPORTS
      #define JLCXX_API __declspec(dllexport)
  #else
      #define JLCXX_API __declspec(dllimport)
  #endif
  #define JLCXX_ONLY_EXPORTS __declspec(dllexport)
#else
   #define JLCXX_API __attribute__ ((visibility("default")))
   #define JLCXX_ONLY_EXPORTS JLCXX_API
#endif

#define JLCXX_VERSION_MAJOR 0
#define JLCXX_VERSION_MINOR 14
#define JLCXX_VERSION_PATCH 2

// From https://stackoverflow.com/questions/5459868/concatenate-int-to-string-using-c-preprocessor
#define __JLCXX_STR_HELPER(x) #x
#define __JLCXX_STR(x) __JLCXX_STR_HELPER(x)
#define JLCXX_VERSION_STRING __JLCXX_STR(JLCXX_VERSION_MAJOR) "." __JLCXX_STR(JLCXX_VERSION_MINOR) "." __JLCXX_STR(JLCXX_VERSION_PATCH)

// Apple Clang doesn't really support ranges fully until __cpp_lib_ranges==202207L (AppleClang 16)
#if defined(__cpp_lib_ranges) && !defined(JLCXX_FORCE_RANGES_OFF)
#  if (defined(__clang__) && defined(__apple_build_version__)) || defined _MSC_VER
#    if __cpp_lib_ranges >= 202207L
#      define JLCXX_HAS_RANGES
#    endif
#  elif __cpp_lib_ranges >= 201911L
#    define JLCXX_HAS_RANGES
#  endif
#endif

#endif
