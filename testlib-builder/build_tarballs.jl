using BinaryBuilder

sources = [
    "src"
]

# Bash recipe for building across all platforms
script = raw"""
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/opt/$target/$target.toolchain -DCMAKE_CXX_FLAGS="-march=x86-64" -DCMAKE_INSTALL_PREFIX=${prefix} -DCMAKE_FIND_ROOT_PATH=${prefix} -DJulia_PREFIX=${prefix} ../testlib
VERBOSE=ON cmake --build . --config Release --target install
"""

version_number = get(ENV, "TRAVIS_TAG", "")
if version_number == ""
    version_number = "v0.99.0"
end

# This transforms the dependency script to use the binaries generated in the same Travis run. This code should be skipped on a stand-alone project
### begin of part to be skipped ###
@show ws_root = dirname(dirname(dirname(dirname(@__FILE__))))
@show prod_dir = joinpath(ws_root, "products")
@show generated_script_name = joinpath(prod_dir,"build_libcxxwrap-julia-1.0.$(version_number).jl")
local_dep_script = ""
open(generated_script_name, "r") do generated_script
    while !eof(generated_script)
        l = readline(generated_script; keep=true)
        if startswith(l, "bin_prefix = \"https://github.com")
            l = "bin_prefix = \"$prod_dir\"\n"
        end
        global local_dep_script *= l
    end
end
### end of part to be skipped ###

# These are the platforms we will build for by default, unless further
# platforms are passed in on the command line
platforms = Platform[]
_abis(p) = (:gcc7,:gcc8)
_archs(p) = (:x86_64, :i686)
_archs(::Type{Linux}) = (:x86_64,)
for p in (Linux,Windows)
    for a in _archs(p)
        for abi in _abis(p)
            push!(platforms, p(a, compiler_abi=CompilerABI(abi,:cxx11)))
        end
    end
end
push!(platforms, MacOS(:x86_64))

# The products that we will ensure are always built
products = prefix -> [
    LibraryProduct(prefix, "testlib", :testlib),
]

# Dependencies that must be installed before this package can be built
dependencies = [
   "https://github.com/JuliaPackaging/JuliaBuilder/releases/download/v1.0.0-2/build_Julia.v1.0.0.jl",
   BinaryBuilder.InlineBuildDependency(local_dep_script) # Replace this with build script from https://github.com/JuliaInterop/libcxxwrap-julia/releases
]

build_tarballs(ARGS, "testlib", VersionNumber(version_number), sources, script, platforms, products, dependencies)
