# Adapted from the Yggdrasil build script
# Note that this script can accept some limited command-line arguments, run
# `julia build_tarballs.jl --help` to see a usage message.
using BinaryBuilder, Pkg, TOML

function getversion(headerfile)
    a = b = c = 0
    for l in readlines(headerfile)
        if startswith(l, "#define JLCXX_VERSION_MAJOR")
            a = parse(Int, split(l)[end])
        end
        if startswith(l, "#define JLCXX_VERSION_MINOR")
            b = parse(Int, split(l)[end])
        end
        if startswith(l, "#define JLCXX_VERSION_PATCH")
            c = parse(Int, split(l)[end])
            break
        end
    end
    return VersionNumber(a, b, c)
end

basepath = dirname(@__DIR__)

name = "libcxxwrap_julia"
version = getversion(joinpath(basepath, "include", "jlcxx", "jlcxx_config.hpp"))

julia_versions = [v"1.6.0", v"1.7.0", v"1.8.0"]

# Collection of sources required to complete build
sources = [
    DirectorySource(basepath; target = "libcxxwrap-julia")
]

# Bash recipe for building across all platforms
script = raw"""
mkdir build
cd build

cmake \
    -DJulia_PREFIX=$prefix \
    -DCMAKE_INSTALL_PREFIX=$prefix \
    -DCMAKE_FIND_ROOT_PATH=$prefix \
    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TARGET_TOOLCHAIN} \
    -DCMAKE_BUILD_TYPE=Release \
    ../libcxxwrap-julia
VERBOSE=ON cmake --build . --config Release --target install -- -j${nproc}
install_license $WORKSPACE/srcdir/libcxxwrap-julia*/LICENSE.md
"""

# These are the platforms we will build for by default, unless further
# platforms are passed in on the command line

function libjulia_platforms(julia_version)
    platforms = supported_platforms(; experimental = julia_version ≥ v"1.7")

    filter!(p -> libc(p) != "musl" && arch(p) != "i686" && !contains(arch(p), "arm") && arch(p) != "aarch64" && arch(p) != "powerpc64le" && !Sys.isfreebsd(p), platforms)

    for p in platforms
        p["julia_version"] = string(julia_version)
    end

    platforms = expand_cxxstring_abis(platforms)

    filter!(p -> cxxstring_abi(p) != "cxx03", platforms)

    return platforms
end

platforms = vcat(libjulia_platforms.(julia_versions)...)

# The products that we will ensure are always built
products = [
    LibraryProduct("libcxxwrap_julia", :libcxxwrap_julia; dlopen_flags = [:RTLD_GLOBAL]),
    LibraryProduct("libcxxwrap_julia_stl", :libcxxwrap_julia_stl; dlopen_flags = [:RTLD_GLOBAL]),
]

# Dependencies that must be installed before this package can be built
dependencies = [
    BuildDependency("libjulia_jll"),
]

GITHUB_REF_NAME = haskey(ENV, "GITHUB_REF_NAME") ? ENV["GITHUB_REF_NAME"] : ""
deployingargs = deepcopy(ARGS)
if !isempty(GITHUB_REF_NAME) && GITHUB_REF_NAME != "main"
    push!(deployingargs, "--deploy=local")
end

# Build the tarballs, and possibly a `build.jl` as well.
build_tarballs(deployingargs, name, version, sources, script, platforms, products, dependencies;
    preferred_gcc_version = v"9", julia_compat = "1.6")

if GITHUB_REF_NAME == "main"
    const testreg = "https://github.com/barche/CxxWrapTestRegistry.git"

    const repo = "barche/libcxxwrap_julia_jll.jl"

    Pkg.Registry.add(RegistrySpec(url = testreg))
    Pkg.develop(PackageSpec(url = "https://github.com/$(repo).git"))

    libcxxwrap_jll_project_toml = TOML.parsefile(joinpath(Pkg.devdir(), "libcxxwrap_julia_jll", "Project.toml"))
    build_version = parse(VersionNumber, libcxxwrap_jll_project_toml["version"])
    if version == VersionNumber(build_version.major, build_version.minor, build_version.patch)
        build_version = VersionNumber(build_version.major, build_version.minor, build_version.patch, (), build_version.build .+ 1)
    else
        build_version = version
    end

    mktemp() do jsonfile, _
        push!(deployingargs, "--meta-json=$jsonfile")
        build_tarballs(deployingargs, name, version, sources, script, platforms, products, dependencies;
            preferred_gcc_version = v"9", julia_compat = "1.6")

        json = String(read(jsonfile))
        buff = IOBuffer(strip(json))
        objs = []
        while !eof(buff)
            push!(objs, BinaryBuilder.JSON.parse(buff))
        end
        json_obj = objs[1]

        BinaryBuilder.cleanup_merged_object!(json_obj)
        json_obj["dependencies"] = Dependency[dep for dep in json_obj["dependencies"] if !isa(dep, BuildDependency)]

        tag = "$(name)-v$(build_version)"
        upload_prefix = "https://github.com/$(repo)/releases/download/$(tag)"

        download_dir = "products"
        BinaryBuilder.rebuild_jll_package(json_obj; download_dir, upload_prefix, build_version)
        BinaryBuilder.push_jll_package(name, build_version; deploy_repo = repo)
        BinaryBuilder.upload_to_github_releases(repo, tag, download_dir; verbose = true)
    end

    import libcxxwrap_julia_jll
    using LocalRegistry
    register(libcxxwrap_julia_jll, registry = testreg)
end
