#ifndef JLCXX_JULIA_HEADERS_HPP
#define JLCXX_JULIA_HEADERS_HPP

#ifdef _MSC_VER
    #include <uv.h>
    #include <windows.h>
#endif

// Type wrapping relies on function local static var's in function
// templates, so all template function instantiations must be visible to avoid
// duplicating wrapped types.
// Implicit template instantiations have the visibility of the least visible
// of the template function and template arguments, so an exported/visible template
// with a template argument which includes any type without a visibility
// attribute will cause the visibility of that specific instantiation to match
// the current visibility (e.g. "hidden" for -fvisibility=hidden or from a
// pragma).
// jl_value_t and jl_datatype_t don't have visibility attributes.
// Attributes cannot be added to an already defined type, so the visibility must
// be adjusted for the Julia types/headers.
#ifndef _WIN32
#if defined(JULIA_H) && !defined(FORCE_ALLOW_PRIOR_JULIA_INCLUDE)
// Pre-including Julia headers with hidden-by-default symbol visibility will
// break the type registry. Avoid this error with one of the following
// solutions:
// - Include Julia headers after libcxxwrap headers
// - Set -DFORCE_ALLOW_PRIOR_JULIA_INCLUDE if you have ensured that Julia
// headers are included with default symbol visibility. For example:
//   - Wrap the include with `#pragma GCC visibility push(default)` and
//   `#pragma GCC visibility pop`
//   - Compile with default visibility
// The JlCxx package config defines FORCE_ALLOW_PRIOR_JULIA_INCLUDE for you when the
// consuming target's CXX_VISIBILITY_PRESET is unset or "default".
#error "Including <julia.h> before jlcxx headers may break the type registry. See the comments above this #error for solutions."
#endif

#pragma GCC visibility push(default)
#endif

#include <julia.h>
#include <julia_threads.h>

#ifndef _WIN32
#pragma GCC visibility pop
#endif

#endif
