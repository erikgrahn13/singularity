include_guard(GLOBAL)

cmake_path(GET CMAKE_CURRENT_LIST_DIR PARENT_PATH _singularity_root_dir)
set(SINGULARITY_ROOT_DIR "${_singularity_root_dir}" CACHE INTERNAL "" FORCE)

find_package(Qt6 6.7 REQUIRED COMPONENTS Quick)
qt_standard_project_setup(REQUIRES 6.7)


function(singularity_create_plugin target)
    set(oneValueArgs
        VENDOR BUNDLE_ID URL EMAIL PLUGIN_CLASS PLUGIN_CLASS_HEADER PLUGIN_NAME
        CAPI_SDK_DIR CAPI_MAX_BLOCK_SIZE CAPI_STACK_SIZE)
    set(multiValueArgs SOURCES QML_FILES FORMATS RESOURCES DATA_RESOURCES CAPI_INCLUDE_DIRS)

    # Parse the arguments
    cmake_parse_arguments(PARAMS "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})


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
        URI Singularity.${target}
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
        PLUGIN_CLASS_HEADER="${PARAMS_PLUGIN_CLASS_HEADER}"
        PLUGIN_CLASS=${PARAMS_PLUGIN_CLASS}
        SINGULARITY_QML_MODULE_URI="Singularity.${target}"
        $<$<CONFIG:Debug>:SINGULARITY_QML_SOURCE_FILE="${CMAKE_CURRENT_SOURCE_DIR}/Main.qml">
    )

    target_compile_features(${target} PUBLIC cxx_std_23)

    include(FetchContent)

    foreach(FORMAT IN LISTS PARAMS_FORMATS)
        if(FORMAT STREQUAL "APP")
            include("${SINGULARITY_ROOT_DIR}/standalone/SingularityApp2.cmake")
            singularity_create_app_plugin(${target})
        elseif(FORMAT STREQUAL "VST3")
            include("${SINGULARITY_ROOT_DIR}/vst3/SingularityVst3.cmake")
            singularity_create_vst3_plugin(${target}
                PLUGIN_TITLE "${PARAMS_PLUGIN_NAME}"
            )
        endif()
    endforeach()
endfunction()
