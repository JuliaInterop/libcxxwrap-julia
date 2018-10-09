# JlCxx

[![Build Status](https://travis-ci.org/JuliaInterop/libcxxwrap-julia.svg?branch=master)](https://travis-ci.org/JuliaInterop/libcxxwrap-julia.svg)

This is the C++ library component of the [CxxWrap.jl](https://github.com/JuliaInterop/CxxWrap.jl) package, distributed as a regular CMake library
for use in other C++ projects. The main CMake option of interest is `Julia_EXECUTABLE`, which should point to the Julia executable against which 
the library is to be used. See the [CxxWrap.jl](https://github.com/JuliaInterop/CxxWrap.jl) README for more info.