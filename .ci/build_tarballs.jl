using BinaryBuilder

# Collection of sources required to build Ogg
sources = [
    "JuliaInterop"
]

# Bash recipe for building across all platforms
function getscript(version)
    shortversion = version[1:3]
    return """
    Julia_ROOT=/usr/local

    apk add p7zip

    # Download julia
    cd /usr/local
    curl -L "https://github.com/barche/julia-binaries/releases/download/$version/julia-$version-\$target.tar.gz" | tar -zx --strip-components=1 
    
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
platforms = [
    #Linux(:i686),
    Linux(:x86_64),
    MacOS(:x86_64),
    Windows(:i686),
    Windows(:x86_64)
]

# The products that we will ensure are always built
products = prefix -> [
    LibraryProduct(prefix, "libcxxwrap", :libcxxwrap),
]

# Dependencies that must be installed before this package can be built
dependencies = [
]

# Build the tarballs, and possibly a `build.jl` as well.
version_number = get(ENV, "TRAVIS_TAG", "")
if version_number == ""
    version_number = "v0.99"
end
download_info_07 = build_tarballs(ARGS, "libcxxwrap-julia-0.7", VersionNumber(version_number), sources, getscript("0.7.0-beta"), platforms, products, dependencies)
@show download_info_07
