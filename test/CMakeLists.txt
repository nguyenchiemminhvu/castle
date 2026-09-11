# ---------------------------------------------------------------------------
# Castle - unit tests
# ---------------------------------------------------------------------------

# ---------------------------------------------------------------------------
# GoogleTest
# ---------------------------------------------------------------------------
if(CASTLE_FETCH_GTEST)
    include(FetchContent)

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.15.2
    )

    # Prevent GoogleTest from overriding compiler/linker options on Windows.
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(googletest)
else()
    find_package(GTest REQUIRED)
endif()


# ---------------------------------------------------------------------------
# Coverage option
# ---------------------------------------------------------------------------
option(
    CASTLE_ENABLE_COVERAGE
    "Enable lcov/gcov code coverage instrumentation for the test target"
    OFF
)


# ---------------------------------------------------------------------------
# Collect every test translation unit (mirrors include/castle structure)
# ---------------------------------------------------------------------------
file(GLOB_RECURSE CASTLE_TEST_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)


# ---------------------------------------------------------------------------
# Single test executable
# ---------------------------------------------------------------------------
add_executable(
    castle_tests
    ${CASTLE_TEST_SOURCES}
)

target_link_libraries(
    castle_tests
    PRIVATE
        castle
        GTest::gtest
)

castle_set_cxx_standard(castle_tests 17)

target_compile_options(
    castle_tests
    PRIVATE
        -Wall
        -Wextra
)

if(CASTLE_ENABLE_COVERAGE)
    target_compile_options(
        castle_tests
        PRIVATE
            -O0
            -g
            --coverage
            -fprofile-arcs
            -ftest-coverage
    )
    target_link_libraries(
        castle_tests
        PRIVATE
            --coverage
    )
endif()

include(GoogleTest)
gtest_discover_tests(castle_tests)
