set(CASTLE_EVENT_BENCHMARK_SOURCES

    event_dispatcher.cpp
)

add_executable(
    castle_benchmark_events

    ${CASTLE_EVENT_BENCHMARK_SOURCES}
)

target_link_libraries(
    castle_benchmark_events
    PRIVATE

        castle_benchmark_common
        benchmark::benchmark_main
)

target_compile_features(
    castle_benchmark_events
    PRIVATE

        cxx_std_17
)

set_target_properties(
    castle_benchmark_events
    PROPERTIES

        RUNTIME_OUTPUT_DIRECTORY
            "${CMAKE_BINARY_DIR}/bin"
)
