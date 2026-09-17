# ============================================================================
# CASTLE events Benchmarks
# ============================================================================

set(CASTLE_EVENTS_BENCHMARK_SOURCES

    delegate_dispatcher.cpp
    event_dispatcher.cpp
    sigslot.cpp
)

add_executable(
    castle_benchmark_events

    ${CASTLE_EVENTS_BENCHMARK_SOURCES}
)

target_include_directories(
    castle_benchmark_events
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
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

if(MSVC)

    target_compile_options(
        castle_benchmark_events
        PRIVATE
            /W4
    )

else()

    target_compile_options(
        castle_benchmark_events
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )

endif()
