file(GLOB CASTLE_SAMPLE_FILES "sample_*.cpp")

foreach(source_file IN LISTS CASTLE_SAMPLE_FILES)
    get_filename_component(sample_name ${source_file} NAME_WE)

    castle_add_executable(
        castle_${sample_name}
        SOURCES
            ${source_file}
    )
endforeach()