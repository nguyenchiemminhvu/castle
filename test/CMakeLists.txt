# ---------------------------------------------------------------------------
# GoogleTest
# ---------------------------------------------------------------------------

if(CASTLE_FETCH_GTEST)

    include(FetchContent)

    # Required for MSVC/Windows builds.
    set(
        gtest_force_shared_crt
        ON
        CACHE BOOL
        ""
        FORCE
    )

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.17.0
    )

    FetchContent_MakeAvailable(googletest)

else()

    find_package(
        GTest
        CONFIG
        REQUIRED
    )

endif()


# ---------------------------------------------------------------------------
# Castle unit tests
# ---------------------------------------------------------------------------

castle_add_gtest(
    castle_tests
    SOURCES
        main.cpp
        test_bit.cpp
        test_buffers.cpp
        test_callbacks.cpp
        test_design_patterns.cpp
        test_errors.cpp
        test_events.cpp
        test_logging.cpp
        test_math.cpp
        test_types.cpp
)