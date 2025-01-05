#!/bin/bash

OPT_LEVEL="avx2"

for arg in "$@"
do
    case $arg in
        clean)
        echo "Cleaning build directory..."
        rm -rf build
        shift
        ;;
        --opt-level=*)
        OPT_LEVEL="${arg#*=}"
        shift
        ;;
        *)
        # unknown option
        ;;
    esac
done

echo "OPT_LEVEL is set to ${OPT_LEVEL}. Possible options are [generic, avx2, avx512]."

mkdir -p build
rm -rf build/data

cd build
cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release -DFAISS_OPT_LEVEL=${OPT_LEVEL} ..
make -j$(nproc)
