cmake_minimum_required(VERSION 3.20)
project(ParvEB LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PARVEB_ENABLE_SIMD "Enable SIMD accelerated primitives" ON)
option(PARVEB_ENABLE_POOL "Enable memory pool allocator" ON)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options(-Wno-tautological-compare)
endif ()

add_subdirectory(third_party/googletest)
set(QUILL_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(QUILL_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(QUILL_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(QUILL_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/quill)

add_library(parveb STATIC
    src/simd_utils.cpp
)
target_include_directories(parveb PUBLIC include)
if (PARVEB_ENABLE_SIMD)
    target_compile_definitions(parveb PUBLIC PARVEB_ENABLE_SIMD=1)
else ()
    target_compile_definitions(parveb PUBLIC PARVEB_ENABLE_SIMD=0)
endif ()
if (PARVEB_ENABLE_POOL)
    target_compile_definitions(parveb PUBLIC PARVEB_ENABLE_POOL=1)
else ()
    target_compile_definitions(parveb PUBLIC PARVEB_ENABLE_POOL=0)
endif ()

add_library(absl_headers INTERFACE)
target_include_directories(absl_headers INTERFACE third_party/abseil-cpp)

find_package(Boost REQUIRED COMPONENTS container)
add_library(boost_headers INTERFACE)
target_link_libraries(boost_headers INTERFACE Boost::container)

enable_testing()

add_executable(leaf8_test tests/leaf8_test.cpp)
target_include_directories(leaf8_test PRIVATE include)
target_link_libraries(leaf8_test PRIVATE gtest_main parveb quill::quill)
target_compile_definitions(leaf8_test PRIVATE QUILL_ROOT_LOGGER_ONLY)

include(GoogleTest)
gtest_discover_tests(leaf8_test)

add_executable(branch16_test tests/branch16_test.cpp)
target_include_directories(branch16_test PRIVATE include)
target_link_libraries(branch16_test PRIVATE gtest_main parveb quill::quill)
target_compile_definitions(branch16_test PRIVATE QUILL_ROOT_LOGGER_ONLY)
gtest_discover_tests(branch16_test)

add_executable(veb32_test tests/veb32_test.cpp)
target_include_directories(veb32_test PRIVATE include)
target_link_libraries(veb32_test PRIVATE gtest_main parveb quill::quill)
target_compile_definitions(veb32_test PRIVATE QUILL_ROOT_LOGGER_ONLY)
gtest_discover_tests(veb32_test)

add_executable(veb_benchmark src/veb_benchmark.cpp)
target_include_directories(veb_benchmark PRIVATE include)
target_link_libraries(veb_benchmark PRIVATE parveb quill::quill absl_headers boost_headers)
target_compile_definitions(veb_benchmark PRIVATE QUILL_ROOT_LOGGER_ONLY)
