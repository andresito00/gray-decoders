cmake_minimum_required(VERSION 3.18.2)
project(gray-decoders-exe)

# add_subdirectory(decoder)
# add_subdirectory(receiver)
# add_subdirectory(util)

set(decoder_src decode.cpp)
add_executable(decode ${decoder_src})

# target_link_libraries(
  # decode

  # decoder
  # receiver
  # util
# )
