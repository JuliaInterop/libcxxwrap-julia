# JlCxx

![test-linux-mac](https://github.com/JuliaInterop/libcxxwrap-julia/workflows/test-linux-mac/badge.svg) ![test-win](https://github.com/JuliaInterop/libcxxwrap-julia/workflows/test-win/badge.svg)

This is the C++ library component of the [CxxWrap.jl](https://github.com/JuliaInterop/CxxWrap.jl) package, distributed as a regular CMake library
for use in other C++ projects. To build a Julia interface to a C++ library, you need to build against this library and supply the resulting library as a binary dependency to your Julia package. The `testlib-builder` directory contains a complete example of how to build and distribute these binaries, or you can use the [BinaryBuilder.jl](https://github.com/JuliaPackaging/BinaryBuilder.jl) wizard to generate the builder repository.

See the [CxxWrap.jl](https://github.com/JuliaInterop/CxxWrap.jl) README for more info on the API.

## Building libcxxwrap-julia

### Obtaining the source

Simply clone this repository, e.g.:

```bash
git clone https://github.com/JuliaInterop/libcxxwrap-julia.git
```

You can also manage everything directly from Visual Studio Code using "clone git repository" on the start screen.

### Preparing the install location

Julia loads the `libcxxwrap-julia` through the [libcxxwrap_julia_jll](https://github.com/JuliaBinaryWrappers/libcxxwrap_julia_jll.jl) package, which is built automatically by [BinaryBuilder](https://github.com/JuliaPackaging/BinaryBuilder.jl). To tell Julia to use self-compiled binaries, one way is to use an [`override` directory](https://docs.binarybuilder.org/stable/jll/#dev'ed-JLL-packages). To do this, check out `libcxxwrap_julia_jll` for development:

```julia-repl
pkg> develop libcxxwrap_julia_jll
```

Next, import the package and call the `dev_jll` function:

```julia-repl
julia> import libcxxwrap_julia_jll
julia> libcxxwrap_julia_jll.dev_jll()
```

### Configuring and building

The last command should output the path to the devved JLL, e.g. `/home/user/.julia/dev/libcxxwrap_julia_jll`. In that directory, there will be a subdirectory called `override` which will contain the binaries from the Julia artifacts distribution. This directory should be completely emptied and then used as build directory from cmake, e.g. in a terminal do:

```bash
cd /home/user/.julia/dev/libcxxwrap_julia_jll/override
rm -rf *
cmake -DJulia_PREFIX=/home/user/path/to/julia /path/to/source/of/libcxxwrap-julia
cmake --build . --config Release
```

Here, `/path/to/source/of/libcxxwrap-julia` is the cloned git repository.

Building libcxxwrap-julia requires a C++ compiler which supports C++17
(e.g. GCC 7, clang 5; for macOS users that means Xcode 9.3).

The main CMake option of interest is `Julia_PREFIX`, which should point to the Julia installation you want to use. The `PREFIX` is a directory, one containing the `bin` and `lib` directories and so on. If you are using a binary download of Julia (https://julialang.org/downloads/), this is the top-level directory; if you build Julia from source (https://github.com/JuliaLang/julia), it would be the `usr` directory of the repository. Below we will call this directory `/home/user/path/to/julia`, but you should substitute your actual path in the commands below.

Instead of specifying the prefix, it is also possible to directly set the Julia executable, using:

```bash
cmake -DJulia_EXECUTABLE=/home/user/path/to/julia/bin/julia ../libcxxwrap-julia
```

Next, you can build your own code against this by setting the `JlCxx_DIR` CMake variable to the build directory (`libcxxwrap-julia-build`) used above, or add it to the `CMAKE_PREFIX_PATH` CMake variable.

### Using the compiled libcxxwrap-julia in CxxWrap

Following the above instructions, Julia will now automatically use the compiled binaries. You can verify this using:

```julia-repl
julia> using CxxWrap
julia> CxxWrap.prefix_path()
"/home/user/.julia/dev/libcxxwrap_julia_jll/override"
```

An alternative method of using self-compiled binaries is the `Overrides.toml` file, to be placed in `.julia/artifacts`:

```toml
[3eaa8342-bff7-56a5-9981-c04077f7cee7]
libcxxwrap_julia = "/path/to/libcxxwrap-julia-build"
```

More details [here](https://docs.binarybuilder.org/stable/jll/#Non-dev'ed-JLL-packages).

The file can be generated automatically using the `OVERRIDES_PATH`, `OVERRIDE_ROOT` and `APPEND_OVERRIDES_TOML` Cmake options, with the caveat that each CMake run will append again to the file and make it invalid, i.e. this is mostly intended for use on CI (see the appveyor and travis files for examples).

### Building on Windows

On Windows, building is easiest with [Visual Studio 2019](https://visualstudio.microsoft.com/vs/), for which the Community Edition with C++ support is a free download. You can clone the `https://github.com/JuliaInterop/libcxxwrap-julia.git` repository using the [built-in git support](https://docs.microsoft.com/en-us/visualstudio/get-started/tutorial-open-project-from-repo?view=vs-2019), and configure the `Julia_PREFIX` option from the built-in CMake support. See the [Visual Studio docs](https://docs.microsoft.com/en-us/cpp/build/customize-cmake-settings?view=vs-2019) for more info. Use Julia version 1.3.1, otherwise building will fail with "no target architecture" error (see issue [julia/34201](https://github.com/JuliaLang/julia/issues/34201)).

## Binaries for the main branch

Binaries for the main branch are published at https://github.com/barche/libcxxwrap_julia_jll.jl, you can install them (in Pkg mode, hit `]`) using:

```
add https://github.com/barche/libcxxwrap_julia_jll.jl.git
```
