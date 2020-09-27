#!/bin/bash
export CC=/usr/bin/clang
export CXX=/usr/bin/clang++

mkdir -p build

cmake\
 -DCMAKE_EXPORT_COMPILE_COMMANDS=ON\
 -D_CMAKE_TOOLCHAIN_PREFIX=llvm-\
 -Bbuild\

make -C ./build

cp ./build/cpp/decode .
