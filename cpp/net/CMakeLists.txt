cmake_minimum_required(VERSION 3.18.2)

set(net_src
  tcp.cpp
)
add_library(net ${net_src})

# target_link_libraries(receiver)
