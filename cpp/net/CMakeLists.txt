cmake_minimum_required(VERSION 3.18.2)

set(net_src
  linux/tcp.cpp # TODO: target platform directive in CMakeLists for build time inclusion of correct platform dir
  # freertos/tcp.cpp
  # windows/tcp.cpp...
)
add_library(net ${net_src})
