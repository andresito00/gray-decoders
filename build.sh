#!/bin/bash
export CC=/usr/bin/clang
export CXX=/usr/bin/clang++

mkdir -p build

cmake\
 -D_CMAKE_TOOLCHAIN_PREFIX=llvm-\
 -DCMAKE_USER_MAKE_RULES_OVERRIDE=ClangOverrides.txt
