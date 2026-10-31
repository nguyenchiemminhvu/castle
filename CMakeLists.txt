cmake_minimum_required(VERSION 3.10)

project(
    castle
    VERSION 0.1.0
    LANGUAGES CXX
)

# ---------------------------------------------------------------------------
# CMake helpers
# ---------------------------------------------------------------------------

list(APPEND CMAKE_MODULE_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
)

include(helpers)
include(CTest)


# ---------------------------------------------------------------------------
# Castle - header-only library
# ---------------------------------------------------------------------------

add_library(castle INTERFACE)

target_include_directories(
    castle
    INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_features(
    castle
    INTERFACE
        cxx_std_17
)


# ---------------------------------------------------------------------------
# Project options
# ---------------------------------------------------------------------------

option(
    CASTLE_BUILD_SAMPLES
    "Build Castle samples"
    OFF
)

option(
    CASTLE_FETCH_GTEST
    "Fetch GoogleTest automatically"
    OFF
)

option(
    CASTLE_USING_BSD_SOCKET_BASED_HTTP_PROTOCOL
    "Build the BSD-socket based HTTP client code"
    ON
)

option(
    CASTLE_USING_BSD_SOCKET_BASED_DNS_PROTOCOL
    "Build the BSD-socket based DNS client code"
    ON
)

option(
    CASTLE_USING_BSD_SOCKET_BASED_MQTT_PROTOCOL
    "Build the BSD-socket based MQTT client and broker code"
    ON
)

option(
    CASTLE_USING_BSD_SOCKET_BASED_COAP_PROTOCOL
    "Build the BSD-socket based CoAP client and server code"
    ON
)


# ---------------------------------------------------------------------------
# Samples
# ---------------------------------------------------------------------------

if(CASTLE_BUILD_SAMPLES)
    add_subdirectory(samples)
endif()


# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

if(CASTLE_BUILD_TESTING)
    add_subdirectory(test)
endif()