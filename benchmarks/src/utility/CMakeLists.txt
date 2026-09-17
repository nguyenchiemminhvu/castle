# ============================================================================
# CASTLE utility Benchmarks
# ============================================================================

set(CASTLE_UTILITY_BENCHMARK_SOURCES

    bitset.cpp
    pair.cpp
    variant.cpp
)

add_executable(
    castle_benchmark_utility

    ${CASTLE_UTILITY_BENCHMARK_SOURCES}
)

target_include_directories(
    castle_benchmark_utility
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
    castle_benchmark_utility
    PRIVATE

        castle_benchmark_common
        benchmark::benchmark_main
)

target_compile_features(
    castle_benchmark_utility
    PRIVATE

        cxx_std_17
)

set_target_properties(
    castle_benchmark_utility
    PROPERTIES

        RUNTIME_OUTPUT_DIRECTORY
            "${CMAKE_BINARY_DIR}/bin"
)

if(MSVC)

    target_compile_options(
        castle_benchmark_utility
        PRIVATE
            /W4
    )

else()

    target_compile_options(
        castle_benchmark_utility
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )

endif()
