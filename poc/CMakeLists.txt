cmake_minimum_required(VERSION 3.16)
project(castle_poc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_executable(location_service
    src/main.cpp
    src/transport.cpp
    src/ubx_controller.cpp
    src/parsers.cpp
    src/receiver.cpp
    src/consumers.cpp
    src/diagnostic.cpp
    src/gnss_hal.cpp
    src/tick_source.cpp
    src/location_service.cpp
)

target_include_directories(location_service PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/castle/include
)

target_compile_definitions(location_service PRIVATE
    CASTLE_USING_POSIX_APIS=1
    CASTLE_USING_PTHREAD=1
)

target_compile_options(location_service PRIVATE
    -Wall -Wextra -Wpedantic
    -fno-exceptions -fno-rtti
)

target_link_libraries(location_service PRIVATE Threads::Threads)
set_target_properties(location_service PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    OUTPUT_NAME location_service
)

install(TARGETS location_service RUNTIME DESTINATION bin)

configure_file(config/ubx_config.cfg ${CMAKE_BINARY_DIR}/ubx_config.cfg COPYONLY)
