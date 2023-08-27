## Training and Test Data Generation - Python3
The Python source is the first stage in my port from undergrad work I had implemented in Matlab.

It's main functionality is provide users with a way to:
1. simulate random processes that define neuron firing rates
2. visualize the neuron firing rates
3. generate spike rasters according to the random distributions desired
4. send those packed event arrays using a network protocol to a listening decoding process
5. TODO: train the weights of an inference model using generated brain activity and corresponding physical stimiulus.

```
$ python3 nsimulate.py --mode simulate --rates 50 --intervals 2000 --num-trials 100 --rand EXP
```



## Neural Decoding - C++
1. Receive Spike Rasters on a listening interface
2. Deserialize and enqueue the raster structs as work in progress
3. TODO: "Decode" the SpikeRasters using trained weights from Step 5 above and predict and encode the physical stimulus determined by the model.

```
$ ./decode
decode.cpp: 71: Binding to 0.0.0.0: 8808
tcp.cpp: 142: Listening for connection...
tcp.cpp: 177: Connection accpeted, stream open...
```

## Building the project source

The following steps are prepared and implemented in the bash scripts `setup.sh` and `build.sh`:

1. Set your compiler:
```sh
$ export CC=/usr/bin/clang
$ export CXX=/usr/bin/clang++
```

2. Create a destination directory for build artifacts:
```sh
$ mkdir -p build
```

3. Generate the Makefile:
```sh
$ cmake \
 -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
 -D_CMAKE_TOOLCHAIN_PREFIX=llvm- \
 -Bbuild
```

3. Run the generated Makefile
```sh
$ make -C ./build
```

No support for Windows. YMMV with MacOS. Excited to continue adding support for these.
