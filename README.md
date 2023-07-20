To build:

Set your compiler:
```
#!/bin/bash
export CC=/usr/bin/clang
export CXX=/usr/bin/clang++
```

Create a destination directory for build artifacts:
```
mkdir -p build
```

Generate the Makefile:
```
cmake\
 -DCMAKE_EXPORT_COMPILE_COMMANDS=ON\
 -D_CMAKE_TOOLCHAIN_PREFIX=llvm-\
 -Bbuild
```

Run the generated Makefile
```
make -C ./build
```
