cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0135 NEW)

project(IntegralConfig
    VERSION 1.0.0
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(INTEGRALCONFIG_IS_TOP_LEVEL OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(INTEGRALCONFIG_IS_TOP_LEVEL ON)
endif()

option(INTEGRALCONFIG_BUILD_TESTS "Build IntegralConfig tests" ${INTEGRALCONFIG_IS_TOP_LEVEL})
option(INTEGRALCONFIG_BUILD_EXAMPLES "Build IntegralConfig examples" ${INTEGRALCONFIG_IS_TOP_LEVEL})

include(FetchContent)

FetchContent_Declare(
    IntegralCommunication
    GIT_REPOSITORY https://github.com/IntegralMotions/IntegralCommunication.git
    GIT_TAG        v1.0.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(IntegralCommunication)

file(GLOB_RECURSE INTEGRALCONFIG_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.[ch]
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.[ch]pp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.[ch]xx
)

add_library(IntegralConfig ${INTEGRALCONFIG_SOURCES})
add_library(IntegralConfig::IntegralConfig ALIAS IntegralConfig)

target_include_directories(IntegralConfig
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(IntegralConfig PUBLIC cxx_std_17)

target_link_libraries(IntegralConfig
    PUBLIC
        IntegralCommunication::IntegralCommunication
)

if(INTEGRALCONFIG_BUILD_EXAMPLES)
    add_executable(Example examples/desktop/main.cpp)
    target_link_libraries(Example PRIVATE IntegralConfig::IntegralConfig)
endif()

if(INTEGRALCONFIG_BUILD_TESTS)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()

    file(GLOB_RECURSE INTEGRALCONFIG_TEST_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp
    )

    add_executable(IntegralConfigTests ${INTEGRALCONFIG_TEST_SOURCES})

    target_include_directories(IntegralConfigTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )

    target_link_libraries(IntegralConfigTests PRIVATE
        IntegralConfig::IntegralConfig
        GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(IntegralConfigTests)
endif()
