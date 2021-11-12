#ifndef JLCXX_CONFIG_HPP
#define JLCXX_CONFIG_HPP

#ifdef _WIN32
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
#define JLCXX_VERSION_MINOR 9
#define JLCXX_VERSION_PATCH 0

// From https://stackoverflow.com/questions/5459868/concatenate-int-to-string-using-c-preprocessor
#define __JLCXX_STR_HELPER(x) #x
#define __JLCXX_STR(x) __JLCXX_STR_HELPER(x)
#define JLCXX_VERSION_STRING __JLCXX_STR(JLCXX_VERSION_MAJOR) "." __JLCXX_STR(JLCXX_VERSION_MINOR) "." __JLCXX_STR(JLCXX_VERSION_PATCH)

#endif
