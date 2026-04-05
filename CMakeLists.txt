cmake_minimum_required(VERSION 3.16)
project(tinydb LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(TINYDB_BUILD_EXAMPLES "Build tinydb examples" ON)
option(TINYDB_BUILD_TESTS "Build tinydb tests" ON)

add_library(tinydb INTERFACE)
target_include_directories(tinydb INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(tinydb INTERFACE cxx_std_17)
add_library(tinydb::tinydb ALIAS tinydb)

if(TINYDB_BUILD_EXAMPLES)
    add_executable(tinydb_example examples/basic.cpp)
    target_link_libraries(tinydb_example PRIVATE tinydb)

    add_executable(tinydb_benchmark examples/benchmark.cpp)
    target_link_libraries(tinydb_benchmark PRIVATE tinydb)
endif()

if(TINYDB_BUILD_TESTS)
    enable_testing()
    add_executable(tinydb_tests tests/test_basic.cpp)
    target_link_libraries(tinydb_tests PRIVATE tinydb)
    add_test(NAME tinydb_tests COMMAND tinydb_tests)
endif()
