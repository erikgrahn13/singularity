include_guard(GLOBAL)

function(singularity_create_capi_plugin target)
    set(oneValueArgs
        PLUGIN_CLASS
        PLUGIN_CLASS_HEADER
        SDK_DIR
        MAX_BLOCK_SIZE
        STACK_SIZE
        SOURCE_DIR
        BINARY_DIR)
    set(multiValueArgs SOURCES INCLUDE_DIRS)
    cmake_parse_arguments(CAPI "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT CAPI_PLUGIN_CLASS OR NOT CAPI_PLUGIN_CLASS_HEADER)
        message(FATAL_ERROR
            "[singularity] CAPI target '${target}' requires PLUGIN_CLASS and PLUGIN_CLASS_HEADER.")
    endif()

    if(NOT target MATCHES "^[A-Za-z_][A-Za-z0-9_]*$")
        message(FATAL_ERROR
            "[singularity] CAPI target name must be a valid C identifier because it is used "
            "as the exported entry-point prefix; got '${target}'.")
    endif()

    if(NOT CAPI_MAX_BLOCK_SIZE)
        set(CAPI_MAX_BLOCK_SIZE 4096)
    endif()
    if(NOT CAPI_STACK_SIZE)
        set(CAPI_STACK_SIZE 4096)
    endif()
    if(NOT CAPI_MAX_BLOCK_SIZE MATCHES "^[1-9][0-9]*$")
        message(FATAL_ERROR
            "[singularity] CAPI_MAX_BLOCK_SIZE must be a positive integer.")
    endif()
    if(NOT CAPI_STACK_SIZE MATCHES "^[1-9][0-9]*$")
        message(FATAL_ERROR
            "[singularity] CAPI_STACK_SIZE must be a positive integer.")
    endif()

    set(_capi_include_dirs ${CAPI_INCLUDE_DIRS})
    if(NOT _capi_include_dirs)
        set(_capi_sdk_root "${CAPI_SDK_DIR}")
        if(NOT _capi_sdk_root AND SINGULARITY_CAPI_SDK_DIR)
            set(_capi_sdk_root "${SINGULARITY_CAPI_SDK_DIR}")
        endif()
        if(NOT _capi_sdk_root)
            message(FATAL_ERROR
                "[singularity] FORMAT CAPI requires CAPI_SDK_DIR, SINGULARITY_CAPI_SDK_DIR, "
                "or CAPI_INCLUDE_DIRS for the AudioReach CAPI headers.")
        endif()

        set(_capi_include_dirs
            "${_capi_sdk_root}/ar_osal/api"
            "${_capi_sdk_root}/fwk/api/modules"
            "${_capi_sdk_root}/fwk/spf/interfaces/module/capi"
            "${_capi_sdk_root}/fwk/api/ar_utils"
            "${_capi_sdk_root}/fwk/spf/interfaces/module/metadata/api"
        )
    endif()

    unset(_capi_header_dir CACHE)
    find_path(_capi_header_dir capi.h PATHS ${_capi_include_dirs} NO_DEFAULT_PATH)
    if(NOT _capi_header_dir)
        message(FATAL_ERROR
            "[singularity] Could not find capi.h in the configured CAPI include directories.")
    endif()

    set(SINGULARITY_CAPI_TARGET_NAME "${target}")
    set(SINGULARITY_CAPI_PLUGIN_CLASS "${CAPI_PLUGIN_CLASS}")
    set(SINGULARITY_CAPI_PLUGIN_CLASS_HEADER "${CAPI_PLUGIN_CLASS_HEADER}")
    set(_generated_capi_entry "${CAPI_BINARY_DIR}/${target}_capi_entry.cpp")
    configure_file(
        "${SINGULARITY_ROOT_DIR}/capi/SingularityCapiEntry.cpp.in"
        "${_generated_capi_entry}"
        @ONLY
    )

    add_library(${target}_CAPI SHARED
        ${CAPI_SOURCES}
        "${_generated_capi_entry}"
    )
    target_compile_features(${target}_CAPI PRIVATE cxx_std_23)
    target_compile_definitions(${target}_CAPI PRIVATE
        SINGULARITY_CAPI=1
        SINGULARITY_CAPI_MAX_BLOCK_SIZE=${CAPI_MAX_BLOCK_SIZE}
        SINGULARITY_CAPI_STACK_SIZE=${CAPI_STACK_SIZE}
    )
    target_include_directories(${target}_CAPI PRIVATE
        ${SINGULARITY_ROOT_DIR}
        ${SINGULARITY_ROOT_DIR}/capi
        ${CAPI_SOURCE_DIR}
        ${_capi_include_dirs}
    )
    set_target_properties(${target}_CAPI PROPERTIES
        OUTPUT_NAME "${target}"
        POSITION_INDEPENDENT_CODE ON
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
        LIBRARY_OUTPUT_DIRECTORY "${CAPI_BINARY_DIR}/out/CAPI/$<CONFIG>"
    )
endfunction()
