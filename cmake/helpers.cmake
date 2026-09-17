include_guard(GLOBAL)

function(castle_set_cxx_standard target standard)
    set_target_properties(
        ${target}
        PROPERTIES
            CXX_STANDARD ${standard}
            CXX_STANDARD_REQUIRED YES
            CXX_EXTENSIONS NO
    )
endfunction()


function(castle_enable_warnings target)
    target_compile_options(
        ${target}
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
endfunction()


function(castle_disable_features target feature)
    if(${feature} STREQUAL "exceptions")
        target_compile_options(${target} PRIVATE -fno-exceptions)

    elseif(${feature} STREQUAL "rtti")
        target_compile_options(${target} PRIVATE -fno-rtti)

    elseif(${feature} STREQUAL "all")
        castle_disable_features(${target} "exceptions")
        castle_disable_features(${target} "rtti")
    endif()
endfunction()


function(castle_add_gtest target)
    cmake_parse_arguments(
        CASTLE
        ""
        ""
        "SOURCES"
        ${ARGN}
    )

    add_executable(
        ${target}
        ${CASTLE_SOURCES}
    )

    target_link_libraries(
        ${target}
        PRIVATE
            castle
            GTest::gtest_main
    )

    castle_set_cxx_standard(${target} 17)
    castle_enable_warnings(${target})

    include(GoogleTest)

    gtest_discover_tests(${target})
endfunction()


function(castle_add_executable target)
    cmake_parse_arguments(
        CASTLE
        ""
        ""
        "SOURCES"
        ${ARGN}
    )

    add_executable(
        ${target}
        ${CASTLE_SOURCES}
    )

    target_link_libraries(
        ${target}
        PRIVATE
            castle
            pthread
    )

    castle_set_cxx_standard(${target} 17)
    castle_enable_warnings(${target})
    castle_disable_features(${target} "exceptions")
    castle_disable_features(${target} "rtti")
endfunction()
