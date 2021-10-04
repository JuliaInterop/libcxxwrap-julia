#ifndef JLCXX_JULIA_HEADERS_HPP
#define JLCXX_JULIA_HEADERS_HPP

#ifdef _MSC_VER
    #include <uv.h>
    #include <windows.h>

    template<typename T>
    static inline T jl_atomic_load_relaxed(volatile T *obj)
    {
        return jl_atomic_load_acquire(obj);
    }
#endif

#include <julia.h>
#include <julia_threads.h>

#endif
