include_guard(GLOBAL)
cmake_minimum_required(VERSION 3.22)

set(SINGULARITY_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "")


function(singularity_create_plugin target)
    set(oneValueArgs PACKAGE_NAME PLUGIN_WIDTH PLUGIN_HEIGHT)
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
    set(PLUGIN_WIDTH ${PARAMS_PLUGIN_WIDTH})
    set(PLUGIN_HEIGHT ${PARAMS_PLUGIN_HEIGHT})

    if(NOT SOURCES AND NOT PARAMS_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "No sources provided for target '${target}'. You must provide at least one source file.")
    endif()

    if(NOT SOURCES)
        set(SOURCES ${PARAMS_UNPARSED_ARGUMENTS})
    endif()

    add_library(${target}-shared STATIC 
        ${SOURCES}
        ${SINGULARITY_ROOT_DIR}/SingularityGraphics2.cpp
        ${SINGULARITY_ROOT_DIR}/chocFileWatcher.cpp
        ${SINGULARITY_ROOT_DIR}/SkiaRenderer.cpp
        ${SINGULARITY_ROOT_DIR}/QuickJSEngine.cpp
    )

    target_link_libraries(${target}-shared PRIVATE 
        qjs-libc
        choc::choc
        ${SINGULARITY_SKIA_LIB}
    )

    target_include_directories(${target}-shared PRIVATE
        ${SINGULARITY_SKIA_INCLUDE_DIR}
        ${SINGULARITY_QUICKJS_DIR}
    )

    # Inside the function, after resolving UI paths:
    # list(GET ABSOLUTE_UI 0 FIRST_UI_FILE)
    # cmake_path(GET FIRST_UI_FILE PARENT_PATH UI_DIR)
    list(GET UI 0 UI_MAIN_FILE)
    if(NOT IS_ABSOLUTE "${UI_MAIN_FILE}")
        set(UI_MAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${UI_MAIN_FILE}")
    endif()

    target_compile_definitions(${target}-shared PUBLIC
        UI_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
        UI_MAIN="${UI_MAIN_FILE}"
        PLUGIN_WIDTH=${PLUGIN_WIDTH}
        PLUGIN_HEIGHT=${PLUGIN_HEIGHT}
    )
    target_compile_definitions(${target}-shared PRIVATE
        SINGULARITY_FRAMEWORK_DIR="${SINGULARITY_ROOT_DIR}"
    )

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
            target_compile_features(${target}_APP PRIVATE cxx_std_23)
            set_target_properties(${target}_APP PROPERTIES OUTPUT_NAME ${target})
            target_link_libraries(${target}_APP PRIVATE ${target}-shared)
        elseif(type STREQUAL "VST3")
            # set(CMAKE_OSX_DEPLOYMENT_TARGET "12.7" CACHE STRING "Minimum OS X deployment version" FORCE)
            set(CMAKE_OSX_DEPLOYMENT_TARGET 10.13 CACHE STRING "")

            smtg_add_vst3plugin(${target}_VST3
                PACKAGE_NAME ${target}
                ${SINGULARITY_ROOT_DIR}/vst3/version.h
                ${SINGULARITY_ROOT_DIR}/vst3/helloworldcids.h
                ${SINGULARITY_ROOT_DIR}/vst3/helloworldprocessor.h
                ${SINGULARITY_ROOT_DIR}/vst3/helloworldprocessor.cpp
                ${SINGULARITY_ROOT_DIR}/vst3/helloworldcontroller.h
                ${SINGULARITY_ROOT_DIR}/vst3/helloworldcontroller.cpp
                ${SINGULARITY_ROOT_DIR}/vst3/helloworldentry.cpp
                ${SINGULARITY_ROOT_DIR}/vst3/SingularityView.h
                ${SINGULARITY_ROOT_DIR}/vst3/SingularityView.cpp
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
            )

            target_compile_features(${target}_VST3 PRIVATE cxx_std_23)

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

    # target_compile_definitions(${target} PUBLIC
    #     PLUGIN_WIDTH=${PARAMS_PLUGIN_WIDTH}
    #     PLUGIN_HEIGHT=${PARAMS_PLUGIN_HEIGHT}
    # )

    
endfunction()