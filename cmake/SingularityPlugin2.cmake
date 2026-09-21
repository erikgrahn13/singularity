include_guard(GLOBAL)

cmake_path(GET CMAKE_CURRENT_LIST_DIR PARENT_PATH _singularity_root_dir)
set(SINGULARITY_ROOT_DIR "${_singularity_root_dir}" CACHE INTERNAL "" FORCE)
include(FetchContent)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux"
        AND CMAKE_BUILD_TYPE STREQUAL "Release")
    FetchContent_Declare(
        patchelf_0191
        URL
            "https://github.com/NixOS/patchelf/releases/download/0.19.1/patchelf-0.19.1-x86_64.tar.gz"
        URL_HASH
            SHA256=a6818fef80128fb354423234ecacdcca3e993913d774e5d8346bc63f70fed4cf
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )

    FetchContent_MakeAvailable(patchelf_0191)

    set(QT_DEPLOY_PATCHELF_EXECUTABLE "${patchelf_0191_SOURCE_DIR}/bin/patchelf" CACHE FILEPATH "patchelf used by Qt deployment")
    set(QT_DEPLOY_USE_PATCHELF ON CACHE BOOL "Use patchelf during Qt deployment")

    file(CHMOD "${QT_DEPLOY_PATCHELF_EXECUTABLE}"
        PERMISSIONS
            OWNER_READ OWNER_WRITE OWNER_EXECUTE
            GROUP_READ GROUP_EXECUTE
            WORLD_READ WORLD_EXECUTE
    )
endif()

find_package(Qt6 6.10 REQUIRED COMPONENTS Quick)
qt_standard_project_setup(REQUIRES 6.10)


function(singularity_create_plugin target)
    set(oneValueArgs
        VENDOR BUNDLE_ID URL EMAIL PLUGIN_CLASS PLUGIN_CLASS_HEADER PLUGIN_NAME
        CAPI_SDK_DIR CAPI_MAX_BLOCK_SIZE CAPI_STACK_SIZE)
    set(multiValueArgs SOURCES QML_FILES FORMATS RESOURCES DATA_RESOURCES CAPI_INCLUDE_DIRS)

    # Parse the arguments
    cmake_parse_arguments(PARAMS "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(PARAMS_PLUGIN_NAME)
        set(pluginTitle "${PARAMS_PLUGIN_NAME}")
    else()
        set(pluginTitle "${target}")
    endif()


    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    # qt_add_library(${target} STATIC
    #     ${SINGULARITY_ROOT_DIR}/SingularityController.h
    #     ${SINGULARITY_ROOT_DIR}/SingularityController.cpp
    # )

    set(PLUGIN_VIEW "${SINGULARITY_ROOT_DIR}/PluginView.qml")

    set_source_files_properties(
        "${PLUGIN_VIEW}"
        PROPERTIES
            QT_RESOURCE_ALIAS "PluginView.qml"
    )

    qt_add_qml_module(${target}
        STATIC
        URI Singularity
        OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Singularity"
        NO_PLUGIN
        SOURCES
            ${SINGULARITY_ROOT_DIR}/SingularityController.h
            ${SINGULARITY_ROOT_DIR}/SingularityController.cpp
        QML_FILES
            ${PLUGIN_VIEW}
            Main.qml
            ${PARAMS_QML_FILES}
        RESOURCES
            ${PARAMS_RESOURCES}
    )

    target_link_libraries(${target} PUBLIC
        Qt6::Quick
    )

    target_compile_definitions(${target} PUBLIC
        PLUGIN_NAME="${pluginTitle}"
        PLUGIN_CLASS_HEADER="${PARAMS_PLUGIN_CLASS_HEADER}"
        PLUGIN_CLASS=${PARAMS_PLUGIN_CLASS}
        SINGULARITY_QML_MODULE_URI="Singularity"
    )

    target_compile_features(${target} PUBLIC cxx_std_23)

    foreach(FORMAT IN LISTS PARAMS_FORMATS)
        if(FORMAT STREQUAL "APP")
            include("${SINGULARITY_ROOT_DIR}/standalone/SingularityApp2.cmake")
            singularity_create_app_plugin(${target})
        elseif(FORMAT STREQUAL "VST3")
            include("${SINGULARITY_ROOT_DIR}/vst3/SingularityVst3.cmake")
            singularity_create_vst3_plugin(${target}
                PLUGIN_TITLE "${pluginTitle}"
            )
        endif()
    endforeach()
endfunction()
