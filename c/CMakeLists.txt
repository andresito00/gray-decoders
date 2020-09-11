set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -Wshadow")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -Wpedantic")

set(CMAKE_C_CLANG_TIDY clang-tidy -checks=-*,readability-*)
