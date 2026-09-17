file(GLOB_RECURSE CASTLE_SAMPLE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

foreach(source_file IN LISTS CASTLE_SAMPLE_FILES)

    # File name without extension
    get_filename_component(file_name "${source_file}" NAME_WE)

    # Path relative to source root
    file(RELATIVE_PATH rel_path
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${source_file}"
    )

    # Parent directory of the file
    get_filename_component(sub_dir "${rel_path}" DIRECTORY)

    if(sub_dir)
        # Replace path separators with underscores
        string(REPLACE "/" "_" sub_dir "${sub_dir}")
        string(REPLACE "\\" "_" sub_dir "${sub_dir}")

        set(target_name "${sub_dir}_${file_name}")
    else()
        set(target_name "${file_name}")
    endif()

    castle_add_executable(
        ${target_name}
        SOURCES
            ${source_file}
    )

    target_include_directories(${target_name}
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/helper"
    )

endforeach()