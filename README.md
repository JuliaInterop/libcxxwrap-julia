# JlCxx

[![Build Status](https://travis-ci.org/JuliaInterop/libcxxwrap-julia.svg?branch=master)](https://travis-ci.org/JuliaInterop/libcxxwrap-julia)
[![Build status](https://ci.appveyor.com/api/projects/status/96h6josegra2ct2d?svg=true)](https://ci.appveyor.com/project/barche/libcxxwrap-julia)

This is the C++ library component of the [CxxWrap.jl](https://github.com/JuliaInterop/CxxWrap.jl) package, distributed as a regular CMake library
for use in other C++ projects. The main CMake option of interest is `Julia_PREFIX`, which should point to the installation prefix where the Julia against which 
the library is to be used is installed.

See the [CxxWrap.jl](https://github.com/JuliaInterop/CxxWrap.jl) README for more info.