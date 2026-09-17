# ============================================================================
# CASTLE Container Benchmarks
# ============================================================================

set(CASTLE_CONTAINER_BENCHMARK_SOURCES

    array.cpp
    vector.cpp
    hash_map.cpp
    hash_set.cpp
    tree_map.cpp
    tree_set.cpp
    string.cpp
    string_view.cpp
    stack.cpp
    queue.cpp
)

add_executable(
    castle_benchmark_containers

    ${CASTLE_CONTAINER_BENCHMARK_SOURCES}
)

target_include_directories(
    castle_benchmark_containers
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
    castle_benchmark_containers
    PRIVATE

        castle_benchmark_common
        benchmark::benchmark_main
)

target_compile_features(
    castle_benchmark_containers
    PRIVATE

        cxx_std_17
)

set_target_properties(
    castle_benchmark_containers
    PROPERTIES

        RUNTIME_OUTPUT_DIRECTORY
            "${CMAKE_BINARY_DIR}/bin"
)

if(MSVC)

    target_compile_options(
        castle_benchmark_containers
        PRIVATE
            /W4
    )

else()

    target_compile_options(
        castle_benchmark_containers
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )

endif()
