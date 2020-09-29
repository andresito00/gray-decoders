cmake_minimum_required(VERSION 3.18.2)

set(receiver_src
  receiver_tcp.cpp
)
add_library(receiver ${receiver_src})

target_link_libraries(receiver util)
