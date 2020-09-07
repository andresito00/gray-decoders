cmake_minimum_required(VERSION 3.18.2)

set(util_source_files
  util.cpp
  util.h
  runtime_config.cpp
  runtime_config.h
)

add_library(util ${util_source_files})
