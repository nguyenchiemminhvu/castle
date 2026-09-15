cmake_minimum_required(VERSION 3.16)

project(castle_benchmarks
    LANGUAGES CXX
)

# ============================================================================
# Options
# ============================================================================

option(
    CASTLE_BENCHMARK_ENABLE_STL
    "Enable STL comparison benchmarks"
    ON
)

option(
    CASTLE_BENCHMARK_ENABLE_WARNINGS
    "Enable compiler warnings"
    ON
)

option(
    CASTLE_BENCHMARK_BUILD_CONTAINERS
    "Build container benchmarks"
    ON
)

option(
    CASTLE_BENCHMARK_BUILD_CALLBACKS
    "Build callback benchmarks"
    ON
)

option(
    CASTLE_BENCHMARK_BUILD_EVENTS
    "Build event benchmarks"
    ON
)

option(
    CASTLE_BENCHMARK_BUILD_CHRONO
    "Build chrono benchmarks"
    ON
)

# ============================================================================
# C++
# ============================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================================
# Google Benchmark
# ============================================================================

include(FetchContent)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    google_benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.9.5
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(google_benchmark)

# ============================================================================
# CASTLE
#
# Standalone:
#
#     castle/
#         CMakeLists.txt
#         benchmarks/
#
# If the CASTLE target already exists, use it.
# Otherwise add the parent project.
# ============================================================================

if(NOT TARGET castle)

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../CMakeLists.txt")

        add_subdirectory(
            "${CMAKE_CURRENT_SOURCE_DIR}/.."
            "${CMAKE_CURRENT_BINARY_DIR}/castle"
        )

    else()

        message(FATAL_ERROR
            "CASTLE target not found."
        )

    endif()

endif()

# ============================================================================
# Common benchmark library
# ============================================================================

add_library(castle_benchmark_common INTERFACE)

target_include_directories(
    castle_benchmark_common
    INTERFACE

        "${CMAKE_CURRENT_SOURCE_DIR}/../include"

        "${CMAKE_CURRENT_SOURCE_DIR}/common"
)

target_link_libraries(
    castle_benchmark_common
    INTERFACE

        castle
        benchmark::benchmark
)

target_compile_features(
    castle_benchmark_common
    INTERFACE

        cxx_std_17
)

# ============================================================================
# Benchmark configuration
# ============================================================================

if(CASTLE_BENCHMARK_ENABLE_STL)

    target_compile_definitions(
        castle_benchmark_common
        INTERFACE
            CASTLE_BENCHMARK_ENABLE_STL=1
    )

else()

    target_compile_definitions(
        castle_benchmark_common
        INTERFACE
            CASTLE_BENCHMARK_ENABLE_STL=0
    )

endif()

# ============================================================================
# Warnings
# ============================================================================

if(CASTLE_BENCHMARK_ENABLE_WARNINGS)

    if(MSVC)

        target_compile_options(
            castle_benchmark_common
            INTERFACE
                /W4
        )

    else()

        target_compile_options(
            castle_benchmark_common
            INTERFACE
                -Wall
                -Wextra
                -Wpedantic
        )

    endif()

endif()

# ============================================================================
# Benchmark categories
# ============================================================================

if(CASTLE_BENCHMARK_BUILD_CONTAINERS)

    add_subdirectory(
        src/containers
    )

endif()

if(CASTLE_BENCHMARK_BUILD_CALLBACKS)

    add_subdirectory(
        src/callbacks
    )

endif()

if(CASTLE_BENCHMARK_BUILD_EVENTS)

    add_subdirectory(
        src/events
    )

endif()

if(CASTLE_BENCHMARK_BUILD_CHRONO)

    add_subdirectory(
        src/chrono
    )

endif()

# ============================================================================
# Output
# ============================================================================

set(
    CASTLE_BENCHMARK_OUTPUT_DIRECTORY
    "${CMAKE_BINARY_DIR}/bin"
)

message(STATUS "")
message(STATUS "========================================")
message(STATUS " CASTLE Benchmark Configuration")
message(STATUS "========================================")
message(STATUS "")
message(STATUS "STL comparison : ${CASTLE_BENCHMARK_ENABLE_STL}")
message(STATUS "Warnings       : ${CASTLE_BENCHMARK_ENABLE_WARNINGS}")
message(STATUS "Containers     : ${CASTLE_BENCHMARK_BUILD_CONTAINERS}")
message(STATUS "Callbacks      : ${CASTLE_BENCHMARK_BUILD_CALLBACKS}")
message(STATUS "Events         : ${CASTLE_BENCHMARK_BUILD_EVENTS}")
message(STATUS "Chrono         : ${CASTLE_BENCHMARK_BUILD_CHRONO}")
message(STATUS "")