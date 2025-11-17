cmake_minimum_required(VERSION 3.20)
project(ParvEB LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(third_party/googletest)
set(QUILL_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(QUILL_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(QUILL_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(QUILL_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/quill)

enable_testing()

add_executable(leaf64_test tests/leaf64_test.cpp)
target_include_directories(leaf64_test PRIVATE include)
target_link_libraries(leaf64_test PRIVATE gtest_main quill::quill)
target_compile_definitions(leaf64_test PRIVATE QUILL_ROOT_LOGGER_ONLY)

include(GoogleTest)
gtest_discover_tests(leaf64_test)

add_executable(branch64_test tests/branch64_test.cpp)
target_include_directories(branch64_test PRIVATE include)
target_link_libraries(branch64_test PRIVATE gtest_main quill::quill)
target_compile_definitions(branch64_test PRIVATE QUILL_ROOT_LOGGER_ONLY)
gtest_discover_tests(branch64_test)

add_executable(veb40_test tests/veb40_test.cpp)
target_include_directories(veb40_test PRIVATE include)
target_link_libraries(veb40_test PRIVATE gtest_main quill::quill)
target_compile_definitions(veb40_test PRIVATE QUILL_ROOT_LOGGER_ONLY)
gtest_discover_tests(veb40_test)
