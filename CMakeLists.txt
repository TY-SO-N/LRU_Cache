cmake_minimum_required(VERSION 3.20)
project(lru_cache_simulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(lru_cache_lib
    src/lru_cache.cpp
)
target_include_directories(lru_cache_lib PUBLIC include)

add_executable(lru_cache_simulator
    src/main.cpp
)
target_link_libraries(lru_cache_simulator PRIVATE lru_cache_lib)

add_executable(lru_cache_tests
    tests/test_lru_cache.cpp
)
target_link_libraries(lru_cache_tests PRIVATE lru_cache_lib)

add_executable(lru_cache_benchmark
    benchmarks/benchmark_lru_cache.cpp
)
target_link_libraries(lru_cache_benchmark PRIVATE lru_cache_lib)

enable_testing()
add_test(NAME lru_cache_unit_tests COMMAND lru_cache_tests)
