using BinaryBuilder

# Collection of sources required to build Ogg
sources = [
    "JuliaInterop"
]

# Bash recipe for building across all platforms
function getscript(version)
    shortversion = version[1:3]
    return """
    # Build libcxxwrap
    cd \$WORKSPACE/srcdir/libcxxwrap-julia*
    mkdir build && cd build
    cmake -DJulia_ROOT=\$prefix/julia-$version -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/opt/\$target/\$target.toolchain -DCMAKE_CXX_FLAGS="-march=x86-64" -DCMAKE_INSTALL_PREFIX=\${prefix} ..
    VERBOSE=ON cmake --build . --config Release --target install
    if [[ "\$target" == "x86_64-linux-gnu" ]]; then
        ctest -V
    fi
    """
end

# These are the platforms we will build for by default, unless further
# platforms are passed in on the command line
platforms = [
    Linux(:x86_64),
    MacOS(:x86_64),
    Windows(:i686),
    Windows(:x86_64),
    #Linux(:i686)
]

# The products that we will ensure are always built
products = prefix -> [
    LibraryProduct(prefix, "libcxxwrap", :libcxxwrap),
]

# Dependencies that must be installed before this package can be built
dependencies = [
    "https://github.com/JuliaPackaging/JuliaBuilder/releases/download/1.0.0/build_Julia.v1.0.0.jl"
]

# Build the tarballs, and possibly a `build.jl` as well.
version_number = get(ENV, "TRAVIS_TAG", "")
if version_number == ""
    version_number = "v0.99"
end

build_tarballs(ARGS, "libcxxwrap-julia-1.0", VersionNumber(version_number), sources, getscript("1.0.0"), platforms, products, dependencies)
