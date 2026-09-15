set(CASTLE_CALLBACK_BENCHMARK_SOURCES

    function.cpp
)

add_executable(
    castle_benchmark_callbacks

    ${CASTLE_CALLBACK_BENCHMARK_SOURCES}
)

target_link_libraries(
    castle_benchmark_callbacks
    PRIVATE

        castle_benchmark_common
        benchmark::benchmark_main
)

target_compile_features(
    castle_benchmark_callbacks
    PRIVATE

        cxx_std_17
)

set_target_properties(
    castle_benchmark_callbacks
    PROPERTIES

        RUNTIME_OUTPUT_DIRECTORY
            "${CMAKE_BINARY_DIR}/bin"
)
