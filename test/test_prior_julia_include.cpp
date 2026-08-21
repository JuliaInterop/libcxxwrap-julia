// Compile-only check: The JlCxx CMake package defines
// FORCE_ALLOW_PRIOR_JULIA_INCLUDE when building with default visibility; if
// that stops working, the #error in jlcxx/julia_headers.hpp breaks this build.
#include <julia.h>

#include "jlcxx/jlcxx.hpp"

int main()
{
  return 0;
}
