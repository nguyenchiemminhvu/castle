# ============================================================================
# CASTLE algorithms Benchmarks
# ============================================================================

set(CASTLE_ALGORITHMS_BENCHMARK_SOURCES

    algorithms.cpp
)

add_executable(
    castle_benchmark_algorithms

    ${CASTLE_ALGORITHMS_BENCHMARK_SOURCES}
)

target_include_directories(
    castle_benchmark_algorithms
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
    castle_benchmark_algorithms
    PRIVATE

        castle_benchmark_common
        benchmark::benchmark_main
)

target_compile_features(
    castle_benchmark_algorithms
    PRIVATE

        cxx_std_17
)

set_target_properties(
    castle_benchmark_algorithms
    PROPERTIES

        RUNTIME_OUTPUT_DIRECTORY
            "${CMAKE_BINARY_DIR}/bin"
)

if(MSVC)

    target_compile_options(
        castle_benchmark_algorithms
        PRIVATE
            /W4
    )

else()

    target_compile_options(
        castle_benchmark_algorithms
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )

endif()
