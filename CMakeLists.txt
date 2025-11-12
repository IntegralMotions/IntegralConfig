cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0135 NEW)
project(IntegralConfig LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Library ---
add_library(IntegralConfig src/integral_config.cpp)
target_include_directories(IntegralConfig PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_features(IntegralConfig PUBLIC cxx_std_17)

# --- Example (optional) ---
add_executable(Example examples/desktop/main.cpp)
target_link_libraries(Example PRIVATE IntegralConfig)

# --- GoogleTest (desktop only) ---
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# --- Collect all test files automatically ---
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp
)

add_executable(IntegralConfigTests ${TEST_SOURCES})

target_link_libraries(IntegralConfigTests
    PRIVATE IntegralConfig
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(IntegralConfigTests)
