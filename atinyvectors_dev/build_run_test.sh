#!/bin/bash

if [ "$1" == "clean" ]; then
  echo "Cleaning build directory..."
  rm -rf build
fi

mkdir -p build

cd build
cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug ..
make && ctest --output-on-failure
