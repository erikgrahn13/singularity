include_guard(GLOBAL)
cmake_minimum_required(VERSION 3.22)

set(SINGULARITY_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "")


function(singularity_create_plugin target)
    set(oneValueArgs PACKAGE_NAME VENDOR BUNDLE_ID URL EMAIL PLUGIN_CATEGORY)
    set(multiValueArgs SOURCES UI FORMATS)

    # Parse the arguments
    cmake_parse_arguments(PARAMS "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Now handle the parsed arguments
    # If PACKAGE_NAME is not provided, use the target name as the default
    set(pkg_name "${PARAMS_PACKAGE_NAME}")

    if(NOT pkg_name)
        set(pkg_name ${target})
    endif()

    # If SOURCES is not provided, use the unparsed arguments (if any) as source files
    set(SOURCES "${PARAMS_SOURCES}")
    set(UI "${PARAMS_UI}")
    set(FORMATS "${PARAMS_FORMATS}")
    set(VENDOR ${PARAMS_VENDOR})
    set(URL ${PARAMS_URL})
    set(EMAIL ${PARAMS_EMAIL})
    set(BUNDLE_ID ${PARAMS_BUNDLE_ID})

    if(NOT VENDOR)
        set(VENDOR "Singularity Plugins")
    endif()

    if(NOT BUNDLE_ID)
        set(BUNDLE_ID "com.singularity.example")
    endif()

    if(NOT URL)
        set(URL "https://github.com/erikgrahn13/singularity")
    endif()

    if(NOT EMAIL)
        set(EMAIL "erikgrahn13@gmail.com")
    endif()

    if(NOT SOURCES AND NOT PARAMS_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "No sources provided for target '${target}'. You must provide at least one source file.")
    endif()

    if(NOT SOURCES)
        set(SOURCES ${PARAMS_UNPARSED_ARGUMENTS})
    endif()

    add_library(${target}-shared STATIC 
        ${SOURCES}
        ${SINGULARITY_ROOT_DIR}/SingularityController.cpp
        ${SINGULARITY_ROOT_DIR}/chocFileWatcher.cpp
        ${SINGULARITY_ROOT_DIR}/VisageRenderer2.cpp
        ${SINGULARITY_ROOT_DIR}/QuickJSEngine2.cpp
    )

    target_link_libraries(${target}-shared PUBLIC 
        qjs-libc
        choc::choc
        visage
    )

    target_include_directories(${target}-shared PRIVATE
        ${SINGULARITY_QUICKJS_DIR}
    )

    target_compile_features(${target}-shared PUBLIC cxx_std_23)

    # Inside the function, after resolving UI paths:
    list(GET UI 0 UI_MAIN_FILE)
    if(NOT IS_ABSOLUTE "${UI_MAIN_FILE}")
        set(UI_MAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${UI_MAIN_FILE}")
    endif()

    cmake_path(GET UI_MAIN_FILE STEM _ui_stem)
    target_compile_definitions(${target}-shared PUBLIC
        UI_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
        UI_MAIN="${UI_MAIN_FILE}"
        QSJC_SYMBOL=qjsc_${_ui_stem}
        QSJC_SYMBOL_SIZE=qjsc_${_ui_stem}_size
        PACKAGE_NAME="${pkg_name}"
    )

    # file(GLOB _widget_files "${SINGULARITY_ROOT_DIR}/widgets/*.js")

    # set(_D_args "")
    # foreach(_w ${_widget_files})
    #     list(APPEND _D_args -D ${_w})
    # endforeach()

    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated.h
        COMMAND $<TARGET_FILE:qjsc>
            -M singularity,js_init_module_singularity
            -o ${CMAKE_CURRENT_BINARY_DIR}/generated.h
            ${UI_MAIN_FILE}
        DEPENDS qjsc ${UI_MAIN_FILE}
        VERBATIM
    )

    target_sources(${target}-shared PRIVATE
        $<$<CONFIG:Release>:${CMAKE_CURRENT_BINARY_DIR}/generated.h>
    )

    target_include_directories(${target}-shared PRIVATE
        $<$<CONFIG:Release>:${CMAKE_CURRENT_BINARY_DIR}>
    )

    foreach(type IN LISTS FORMATS)
        # Standalone plugin
        if(type STREQUAL "APP")
            add_executable(${target}_APP 
                ${SINGULARITY_ROOT_DIR}/standalone/main.cpp
                ${SINGULARITY_ROOT_DIR}/standalone/ISingularityAudio.cpp
            )
            if(WIN32)
                target_sources(${target}_APP PRIVATE ${SINGULARITY_ROOT_DIR}/standalone/ASIO.cpp)
                target_compile_definitions(${target}_APP PRIVATE NOMINMAX)
                target_link_libraries(${target}_APP PRIVATE asio)
            elseif(APPLE)
                target_sources(${target}_APP PRIVATE ${SINGULARITY_ROOT_DIR}/standalone/coreAudio.cpp)
                target_link_libraries(${target}_APP PRIVATE 
                    "-framework CoreAudio"
                    "-framework AudioToolbox"
                )
            elseif(UNIX AND NOT APPLE)
                target_sources(${target}_APP PRIVATE ${SINGULARITY_ROOT_DIR}/standalone/PipeWire.cpp)
            endif()

            target_include_directories(${target}_APP PRIVATE ${SINGULARITY_ROOT_DIR}/platform)
            target_compile_definitions(${target}_APP PRIVATE
                SINGULARITY_STANDALONE=1
            )
            set_target_properties(${target}_APP PROPERTIES OUTPUT_NAME ${target})
            target_link_libraries(${target}_APP PRIVATE ${target}-shared)
        elseif(type STREQUAL "VST3")
            # Generate stable UIDs from plugin target name
            set(_uid_seed "${target}")
            set(PLUGIN_CATEGORY "${PARAMS_PLUGIN_CATEGORY}")
            if(NOT PLUGIN_CATEGORY)
                set(PLUGIN_CATEGORY "Fx")
            endif()

            string(MD5 _proc_hash "${_uid_seed}_processor")
            string(MD5 _ctrl_hash "${_uid_seed}_controller")

            string(SUBSTRING "${_proc_hash}" 0  8 PROC_UID_0)
            string(SUBSTRING "${_proc_hash}" 8  8 PROC_UID_1)
            string(SUBSTRING "${_proc_hash}" 16 8 PROC_UID_2)
            string(SUBSTRING "${_proc_hash}" 24 8 PROC_UID_3)
            string(SUBSTRING "${_ctrl_hash}" 0  8 CTRL_UID_0)
            string(SUBSTRING "${_ctrl_hash}" 8  8 CTRL_UID_1)
            string(SUBSTRING "${_ctrl_hash}" 16 8 CTRL_UID_2)
            string(SUBSTRING "${_ctrl_hash}" 24 8 CTRL_UID_3)

            string(TOUPPER "${PROC_UID_0}" PROC_UID_0)
            string(TOUPPER "${PROC_UID_1}" PROC_UID_1)
            string(TOUPPER "${PROC_UID_2}" PROC_UID_2)
            string(TOUPPER "${PROC_UID_3}" PROC_UID_3)
            string(TOUPPER "${CTRL_UID_0}" CTRL_UID_0)
            string(TOUPPER "${CTRL_UID_1}" CTRL_UID_1)
            string(TOUPPER "${CTRL_UID_2}" CTRL_UID_2)
            string(TOUPPER "${CTRL_UID_3}" CTRL_UID_3)

            configure_file(
                "${SINGULARITY_ROOT_DIR}/vst3/vst3plugincids.h.in"
                "${CMAKE_CURRENT_BINARY_DIR}/plugincids.h"
            )
            set(public_sdk_SOURCE_DIR ${SINGULARITY_VST3_PUBLIC_SDK_DIR})
            smtg_add_vst3plugin(${target}_VST3
                PACKAGE_NAME ${target}
                ${SINGULARITY_ROOT_DIR}/vst3/vst3version.h
                ${SINGULARITY_ROOT_DIR}/vst3/vst3processor.h
                ${SINGULARITY_ROOT_DIR}/vst3/vst3processor.cpp
                ${SINGULARITY_ROOT_DIR}/vst3/vst3controller.h
                ${SINGULARITY_ROOT_DIR}/vst3/vst3controller.cpp
                ${SINGULARITY_ROOT_DIR}/vst3/vst3entry.cpp
                ${SINGULARITY_ROOT_DIR}/vst3/SingularityView.h
                ${SINGULARITY_ROOT_DIR}/vst3/SingularityView.cpp
            )

            target_compile_definitions(${target}_VST3 PRIVATE
                PLUGIN_NAME="${target}"
                VENDOR="${VENDOR}"
                URL="${URL}"
                EMAIL="${EMAIL}"
            )

            target_link_libraries(${target}_VST3
                PRIVATE
                    sdk
                    ${target}-shared
            )

            target_include_directories(${target}_VST3 PRIVATE
                ${SINGULARITY_ROOT_DIR}/platform
                ${SINGULARITY_ROOT_DIR}
                ${CMAKE_CURRENT_BINARY_DIR}
            )

            smtg_target_configure_version_file(${target}_VST3)

            if(SMTG_MAC)
                smtg_target_set_bundle(${target}_VST3
                    BUNDLE_IDENTIFIER ${BUNDLE_ID}
                    COMPANY_NAME ${VENDOR}
                )
            elseif(SMTG_WIN)
                target_sources(${target}_VST3 PRIVATE 
                    ${SINGULARITY_ROOT_DIR}/vst3/win32resource.rc
                )
            endif()
        endif()
    endforeach()    
endfunction()