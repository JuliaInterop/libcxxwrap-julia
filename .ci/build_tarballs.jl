using BinaryBuilder

# Collection of sources required to build Ogg
sources = [
    "JuliaInterop"
]

# Bash recipe for building across all platforms
function getscript(version)
    shortversion = version[1:3]
    return """
    Julia_ROOT=\$prefix
    # Julia_ROOT=/usr/local

    # Download julia
    #cd /usr/local
    #curl -L "https://github.com/JuliaPackaging/JuliaBuilder/releases/download/$version/julia-$version-\$target.tar.gz" | tar -zx --strip-components=1 

    # Build libcxxwrap
    cd \$WORKSPACE/srcdir/libcxxwrap-julia*
    mkdir build && cd build
    cmake -DJulia_ROOT=\$Julia_ROOT -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/opt/\$target/\$target.toolchain -DCMAKE_CXX_FLAGS="-march=x86-64" -DCMAKE_INSTALL_PREFIX=\${prefix} ..
    VERBOSE=ON cmake --build . --config Release --target install
    if [[ "\$target" == "x86_64-linux-gnu" ]]; then
        ctest -V
    fi
    """
end

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
    LibraryProduct(prefix, "libcxxwrap", :libcxxwrap),
]

# Dependencies that must be installed before this package can be built
dependencies = [
   "https://github.com/JuliaPackaging/JuliaBuilder/releases/download/v1.0.0-2/build_Julia.v1.0.0.jl"
]

# Build the tarballs, and possibly a `build.jl` as well.
version_number = get(ENV, "TRAVIS_TAG", "")
if version_number == ""
    version_number = "v0.99"
end

build_tarballs(ARGS, "libcxxwrap-julia-1.0", VersionNumber(version_number), sources, getscript("1.0.0"), platforms, products, dependencies)
