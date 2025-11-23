cmake_minimum_required(VERSION 3.20)
project(ParvEB LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PARVEB_ENABLE_SIMD "Enable SIMD accelerated primitives" ON)

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
target_include_directories(parveb PUBLIC
    include
    third_party/unordered_dense/include
)
if (PARVEB_ENABLE_SIMD)
    target_compile_definitions(parveb PUBLIC PARVEB_ENABLE_SIMD=1)
else ()
    target_compile_definitions(parveb PUBLIC PARVEB_ENABLE_SIMD=0)
endif ()
add_library(absl_headers INTERFACE)
target_include_directories(absl_headers INTERFACE third_party/abseil-cpp)

find_package(Boost REQUIRED COMPONENTS container)
add_library(boost_headers INTERFACE)
target_link_libraries(boost_headers INTERFACE Boost::container)

add_subdirectory(tests)

add_executable(veb_benchmark src/veb_benchmark.cpp)
target_include_directories(veb_benchmark PRIVATE include)
target_link_libraries(veb_benchmark PRIVATE parveb quill::quill absl_headers boost_headers)
target_compile_definitions(veb_benchmark PRIVATE QUILL_ROOT_LOGGER_ONLY)
