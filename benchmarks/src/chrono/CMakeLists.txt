set(CASTLE_CHRONO_BENCHMARK_SOURCES

    duration.cpp
)

add_executable(
    castle_benchmark_chrono

    ${CASTLE_CHRONO_BENCHMARK_SOURCES}
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
