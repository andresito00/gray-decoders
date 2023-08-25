cmake_minimum_required (VERSION 3.18.2)
project (gray-decoders)

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  execute_process(COMMAND getconf LEVEL1_DCACHE_LINESIZE OUTPUT_VARIABLE L1_CACHE_LINE_SIZE)
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  execute_process(COMMAND sysctl hw.cachelinesize OUTPUT_VARIABLE L1_CACHE_LINE_SIZE)
endif()
string(REGEX MATCH "([1-9][0-9]*)" L1_CACHE_LINE_SIZE "${CONFIG_L1_CACHE_LINE_SIZE}")

add_subdirectory (cpp)

