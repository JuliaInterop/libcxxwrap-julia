FROM julia:latest

RUN apt-get update && apt-get install -y build-essential libatomic1 python gfortran perl wget m4 cmake pkg-config git

RUN set -eux; \
    git clone https://github.com/JuliaInterop/libcxxwrap-julia.git; \
    mkdir build; \
    cd build; \
    cmake -DCMAKE_BUILD_TYPE:STRING=Debug -DCMAKE_INSTALL_PREFIX=/usr/local -DAPPEND_OVERRIDES_TOML=ON ../libcxxwrap-julia/; \
    cmake --build . --config Debug --target install; \
    sed -i 's!libcxxwrap_julia = .*!libcxxwrap_julia = "/usr/local"!g' ~/.julia/artifacts/Overrides.toml; \
    julia -e 'using Pkg; pkg"add CxxWrap"; pkg"add BinaryBuilder"; pkg"precompile"'; \
    julia -e 'using Pkg; Pkg.test("CxxWrap")'; \
    julia -e 'using CxxWrap; println(CxxWrap.prefix_path())'; \
    rm -rf libcxxwrap-julia build

RUN apt-get update && apt-get install -y ninja-build

RUN git clone https://github.com/barche/libfoo.git; \
    julia --color=yes libfoo/build_tarballs.jl --debug --verbose x86_64-linux-gnu; \
    rm -rf libfoo build products; \
    exit 0

RUN julia -e 'using BinaryBuilder, Pkg.Artifacts; \
              toml = joinpath(dirname(dirname(pathof(BinaryBuilder))), "Artifacts.toml"); \
              ensure_artifact_installed("GCCBootstrap-x86_64-linux-gnu.v4.8.5.x86_64-linux-musl.unpacked", toml; platform=Linux(:x86_64, libc=:musl)); \
              ensure_artifact_installed("GCCBootstrap-x86_64-linux-musl.v4.8.5.x86_64-linux-musl.unpacked", toml; platform=Linux(:x86_64, libc=:musl))'

CMD ["julia"]
