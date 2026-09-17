# ============================================================================
# CASTLE math Benchmarks
# ============================================================================

set(CASTLE_MATH_BENCHMARK_SOURCES

    math.cpp
)

add_executable(
    castle_benchmark_math

    ${CASTLE_MATH_BENCHMARK_SOURCES}
)

target_include_directories(
    castle_benchmark_math
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
    castle_benchmark_math
    PRIVATE

        castle_benchmark_common
        benchmark::benchmark_main
)

target_compile_features(
    castle_benchmark_math
    PRIVATE

        cxx_std_17
)

set_target_properties(
    castle_benchmark_math
    PROPERTIES

        RUNTIME_OUTPUT_DIRECTORY
            "${CMAKE_BINARY_DIR}/bin"
)

if(MSVC)

    target_compile_options(
        castle_benchmark_math
        PRIVATE
            /W4
    )

else()

    target_compile_options(
        castle_benchmark_math
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )

endif()
