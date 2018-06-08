#!/bin/sh
set -e
set -x

mkdir build
cd build
cmake -DJulia_EXECUTABLE=$(which julia) ..
VERBOSE=ON cmake --build . --config Debug --target all
