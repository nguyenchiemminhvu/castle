# ============================================================================
# CASTLE chrono Benchmarks
# ============================================================================

set(CASTLE_CHRONO_BENCHMARK_SOURCES

    duration.cpp
    time_point.cpp
    clocks.cpp
)

add_executable(
    castle_benchmark_chrono

    ${CASTLE_CHRONO_BENCHMARK_SOURCES}
)

target_include_directories(
    castle_benchmark_chrono
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
    castle_benchmark_chrono
    PRIVATE

        castle_benchmark_common
        benchmark::benchmark_main
)

target_compile_features(
    castle_benchmark_chrono
    PRIVATE

        cxx_std_17
)

set_target_properties(
    castle_benchmark_chrono
    PROPERTIES

        RUNTIME_OUTPUT_DIRECTORY
            "${CMAKE_BINARY_DIR}/bin"
)

if(MSVC)

    target_compile_options(
        castle_benchmark_chrono
        PRIVATE
            /W4
    )

else()

    target_compile_options(
        castle_benchmark_chrono
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )

endif()
