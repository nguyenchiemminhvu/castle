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
    if(MSVC)
        target_compile_options(
            ${target}
            PRIVATE
                /W4
                /permissive-
        )
    else()
        target_compile_options(
            ${target}
            PRIVATE
                -Wall
                -Wextra
                -Wpedantic
        )
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
endfunction()
