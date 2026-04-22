include_guard(GLOBAL)
cmake_minimum_required(VERSION 3.22)

set(SINGULARITY_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "")


function(singularity_create_plugin target)
    set(oneValueArgs PACKAGE_NAME VENDOR URL EMAIL PLUGIN_CATEGORY)
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

    if(NOT VENDOR)
        set(VENDOR "Singularity Plugins")
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
        ${SINGULARITY_SKIA_INCLUDE_DIR}
        ${SINGULARITY_QUICKJS_DIR}
    )

    target_compile_features(${target}-shared PUBLIC cxx_std_23)

    # Inside the function, after resolving UI paths:
    # list(GET ABSOLUTE_UI 0 FIRST_UI_FILE)
    # cmake_path(GET FIRST_UI_FILE PARENT_PATH UI_DIR)
    list(GET UI 0 UI_MAIN_FILE)
    if(NOT IS_ABSOLUTE "${UI_MAIN_FILE}")
        set(UI_MAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${UI_MAIN_FILE}")
    endif()

    cmake_path(GET UI_MAIN_FILE STEM _ui_stem)
    message("erik2: ${_ui_stem}")

    target_compile_definitions(${target}-shared PUBLIC
        UI_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
        UI_MAIN="${UI_MAIN_FILE}"
        QSJC_SYMBOL=qjsc_${_ui_stem}
        QSJC_SYMBOL_SIZE=qjsc_${_ui_stem}_size
    )
    target_compile_definitions(${target}-shared PRIVATE
        SINGULARITY_FRAMEWORK_DIR="${SINGULARITY_ROOT_DIR}"
    )

    file(GLOB _widget_files "${SINGULARITY_ROOT_DIR}/widgets/*.js")

    set(_D_args "")
    foreach(_w ${_widget_files})
        list(APPEND _D_args -D ${_w})
    endforeach()

    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated.h
        COMMAND $<TARGET_FILE:qjsc>
            ${_D_args}
            -M native:events,js_events_module_init
            -M native:parameters,js_parameters_module_init
            -M native:audio,js_audio_module_init
            -o ${CMAKE_CURRENT_BINARY_DIR}/generated.h
            ${UI_MAIN_FILE}
        DEPENDS qjsc ${UI_MAIN_FILE} ${_widget_files}
    )

    add_custom_target(${target}_release DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/generated.h)
    add_dependencies(${target}-shared ${target}_release)
    target_include_directories(${target}-shared PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

    foreach(type IN LISTS FORMATS)
        # Standalone plugin
        if(type STREQUAL "APP")
            if(WIN32)
                add_executable(${target}_APP 
                    ${SINGULARITY_ROOT_DIR}/standalone/main.cpp
                    ${SINGULARITY_ROOT_DIR}/standalone/ASIO.cpp
                )
                target_compile_definitions(${target}_APP PRIVATE NOMINMAX)
                # target_sources(${target}_APP PRIVATE standalone/ASIO.cpp standalone/ISingularityAudio.cpp)
                target_link_libraries(${target}_APP PRIVATE asio)
            endif()

            target_sources(${target}_APP PRIVATE ${SINGULARITY_ROOT_DIR}/standalone/ISingularityAudio.cpp)
            target_include_directories(${target}_APP PRIVATE ${SINGULARITY_ROOT_DIR}/platform)
            target_compile_definitions(${target}_APP PRIVATE
                SINGULARITY_STANDALONE=1
                JS_SCRIPTS_DIR="${SINGULARITY_ROOT_DIR}"
            )
            set_target_properties(${target}_APP PROPERTIES OUTPUT_NAME ${target})
            target_link_libraries(${target}_APP PRIVATE ${target}-shared visage)
        elseif(type STREQUAL "VST3")
            # set(CMAKE_OSX_DEPLOYMENT_TARGET "12.7" CACHE STRING "Minimum OS X deployment version" FORCE)
            set(CMAKE_OSX_DEPLOYMENT_TARGET 10.13 CACHE STRING "")

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

            if(APPLE)
                target_sources(${target}_VST3 PRIVATE ${CMAKE_SOURCE_DIR}/platform/macos/CocoaWindow.mm)
            endif()

            target_link_libraries(${target}_VST3
                PRIVATE
                    sdk
                    ${target}-shared
            )

            if(WIN32)
            elseif(APPLE)
                target_link_libraries(${target}_VST3 PRIVATE
                    "-framework AppKit"
                )
            endif()

            target_include_directories(${target}_VST3 PRIVATE
                ${SINGULARITY_ROOT_DIR}/platform
                ${SINGULARITY_ROOT_DIR}
                ${CMAKE_CURRENT_BINARY_DIR}
            )


            smtg_target_configure_version_file(${target}_VST3)

            if(SMTG_MAC)
                smtg_target_set_bundle(${target}_VST3
                    BUNDLE_IDENTIFIER net.steinberg.hello-world
                    COMPANY_NAME "Steinberg Media Technologies GmbH"
                )
                smtg_target_set_debug_executable(${target}_VST3
                    "/Applications/VST3PluginTestHost.app"
                    "--pluginfolder;$(BUILT_PRODUCTS_DIR)"
                )
            elseif(SMTG_WIN)
                target_sources(${target}_VST3 PRIVATE 
                    ${SINGULARITY_ROOT_DIR}/vst3/win32resource.rc
                )
                if(MSVC)
                    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${target}_VST3)

                    smtg_target_set_debug_executable(${target}_VST3
                        "$(ProgramW6432)/Steinberg/VST3PluginTestHost/VST3PluginTestHost.exe"
                        "--pluginfolder \"$(OutDir)/\""
                    )
                endif()
            endif(SMTG_MAC)
        endif()
    endforeach()    
endfunction()