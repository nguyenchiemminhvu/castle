# ============================================================================
# CASTLE callbacks Benchmarks
# ============================================================================

set(CASTLE_CALLBACKS_BENCHMARK_SOURCES

    delegate.cpp
    function.cpp
)

add_executable(
    castle_benchmark_callbacks

    ${CASTLE_CALLBACKS_BENCHMARK_SOURCES}
)

target_include_directories(
    castle_benchmark_callbacks
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
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

if(MSVC)

    target_compile_options(
        castle_benchmark_callbacks
        PRIVATE
            /W4
    )

else()

    target_compile_options(
        castle_benchmark_callbacks
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )

endif()
