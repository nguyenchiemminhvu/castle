# ============================================================================
# CASTLE iterator Benchmarks
# ============================================================================

set(CASTLE_ITERATOR_BENCHMARK_SOURCES

    iterator.cpp
)

add_executable(
    castle_benchmark_iterator

    ${CASTLE_ITERATOR_BENCHMARK_SOURCES}
)

target_include_directories(
    castle_benchmark_iterator
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
    castle_benchmark_iterator
    PRIVATE

        castle_benchmark_common
        benchmark::benchmark_main
)

target_compile_features(
    castle_benchmark_iterator
    PRIVATE

        cxx_std_17
)

set_target_properties(
    castle_benchmark_iterator
    PROPERTIES

        RUNTIME_OUTPUT_DIRECTORY
            "${CMAKE_BINARY_DIR}/bin"
)

if(MSVC)

    target_compile_options(
        castle_benchmark_iterator
        PRIVATE
            /W4
    )

else()

    target_compile_options(
        castle_benchmark_iterator
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )

endif()
