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
            -Werror
    )
endfunction()


function(castle_disable_features target feature)
    if(${feature} STREQUAL "exceptions")
        target_compile_options(${target} PRIVATE -fno-exceptions)
    endif()

    if(${feature} STREQUAL "rtti")
        target_compile_options(${target} PRIVATE -fno-rtti)
    endif()

    if(${feature} STREQUAL "use_stl")
        target_compile_options(${target} PRIVATE -nostdinc++ -fno-builtin -DCASTLE_USING_STD_NEW=0)
        target_compile_options(${target} PRIVATE -DCASTLE_USING_STD_INITIALIZER_LIST=0)
    endif()

    if(${feature} STREQUAL "use_pthread")
        target_compile_options(${target} PRIVATE -DCASTLE_USING_PTHREAD=0)
    endif()
endfunction()


set(CASTLE_BSD_SOCKET_PROTOCOLS http dns mqtt coap)

# Defines CASTLE_USING_BSD_SOCKET_BASED_<PROTOCOL>_PROTOCOL on the target as 1 or 0
# according to the matching CASTLE_USING_BSD_SOCKET_BASED_<PROTOCOL>_PROTOCOL option.
function(castle_add_bsd_socket_protocol_definitions target)
    foreach(protocol IN LISTS CASTLE_BSD_SOCKET_PROTOCOLS)
        string(TOUPPER "${protocol}" upper)

        if(CASTLE_USING_BSD_SOCKET_BASED_${upper}_PROTOCOL)
            set(value 1)
        else()
            set(value 0)
        endif()

        target_compile_definitions(
            ${target}
            PRIVATE
                CASTLE_USING_BSD_SOCKET_BASED_${upper}_PROTOCOL=${value}
        )
    endforeach()
endfunction()


# Removes sources that include the socket-based headers of a disabled protocol.
function(castle_exclude_bsd_socket_sources sources_var)
    set(sources ${${sources_var}})

    foreach(protocol IN LISTS CASTLE_BSD_SOCKET_PROTOCOLS)
        string(TOUPPER "${protocol}" upper)

        if(CASTLE_USING_BSD_SOCKET_BASED_${upper}_PROTOCOL)
            continue()
        endif()

        set(kept_sources)
        foreach(source IN LISTS sources)
            file(
                STRINGS "${source}" socket_includes
                REGEX "#include \"castle_ext/protocols/${protocol}/(client|server|broker|${protocol})\\.hpp\""
            )
            if(NOT socket_includes)
                list(APPEND kept_sources "${source}")
            endif()
        endforeach()

        set(sources ${kept_sources})
    endforeach()

    set(${sources_var} ${sources} PARENT_SCOPE)
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
    castle_disable_features(${target} "use_stl")
    # castle_disable_features(${target} "use_pthread")
endfunction()
