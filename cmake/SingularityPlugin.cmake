include_guard(GLOBAL)
cmake_minimum_required(VERSION 3.22)

set(SINGULARITY_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "")


function(singularity_create_plugin target)
    set(oneValueArgs PACKAGE_NAME VENDOR BUNDLE_ID URL EMAIL PLUGIN_CLASS PLUGIN_CLASS_HEADER)
    set(multiValueArgs SOURCES UI FORMATS RESOURCES DATA_RESOURCES)

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
    set(RESOURCES "${PARAMS_RESOURCES}")
    set(DATA_RESOURCES "${PARAMS_DATA_RESOURCES}")
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

    if(NOT PARAMS_PLUGIN_CLASS_HEADER AND NOT PARAMS_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "No sources provided for target '${target}'. You must provide at least one source file.")
    endif()

    if(NOT SOURCES)
        set(SOURCES ${PARAMS_UNPARSED_ARGUMENTS})
    endif()

    add_library(${target} STATIC 
        ${SOURCES}
        ${SINGULARITY_ROOT_DIR}/SingularityController.cpp
        ${SINGULARITY_ROOT_DIR}/chocFileWatcher.cpp
        ${SINGULARITY_ROOT_DIR}/QuickJSEngine2.cpp
        ${SINGULARITY_ROOT_DIR}/SkiaRenderer2.cpp
    )

    target_link_libraries(${target} PUBLIC 
        qjs-libc
        choc::choc
        ${SINGULARITY_SKIA_LIB}
    )

    if(UNIX AND NOT APPLE)
        target_link_libraries(${target} PUBLIC fontconfig freetype)
    endif()

    target_include_directories(${target} PRIVATE
        ${SINGULARITY_QUICKJS_DIR}
        ${SINGULARITY_SKIA_INCLUDE_DIR}                                    # was: ${skia_SOURCE_DIR}/include
        ${SINGULARITY_SKIA_INCLUDE_DIR}/third_party/externals/dawn/include # was: ${skia_SOURCE_DIR}/include/...
    )

    target_compile_features(${target} PUBLIC cxx_std_23)

    
    # On Windows the MSVC linker emits an import lib (lib/<name>.lib) for both
    # the APP executable and the VST3 DLL.  That collides with the static lib of
    # the same name.  Give the static lib a distinct archive filename so the
    # import libs can use the bare <name>.lib without conflict.
    if(WIN32)
        set_target_properties(${target} PROPERTIES ARCHIVE_OUTPUT_NAME "${target}_shared")
        target_sources(${target} PRIVATE ${SINGULARITY_ROOT_DIR}/platform/windows/Win32Window.cpp)
        target_link_libraries(${target} PUBLIC dxguid.lib)
    elseif(APPLE)
        target_sources(${target} PRIVATE
            ${SINGULARITY_ROOT_DIR}/platform/macos/AppKitWindow.mm
        )
        target_link_libraries(${target} PUBLIC
            "-framework AppKit"
            "-framework Metal"
            "-framework IOKit"
            "-framework IOSurface"
            "-framework QuartzCore"
            "-framework CoreGraphics"
        )
    elseif(UNIX AND NOT APPLE)
        # set_target_properties(${target} PROPERTIES POSITION_INDEPENDENT_CODE ON)
        target_sources(${target} PRIVATE ${SINGULARITY_ROOT_DIR}/platform/linux/X11Window.cpp)
        target_link_libraries(${target} PUBLIC
            X11::X11
            X11::Xrandr
            ${LIBPORTAL_LIBRARIES}
        )
        target_include_directories(${target} PRIVATE ${LIBPORTAL_INCLUDE_DIRS})
    endif()


    # Inside the function, after resolving UI paths:
    list(GET UI 0 UI_MAIN_FILE)
    if(NOT IS_ABSOLUTE "${UI_MAIN_FILE}")
        set(UI_MAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${UI_MAIN_FILE}")
    endif()

    cmake_path(GET UI_MAIN_FILE STEM _ui_stem)

    if(RESOURCES)
        list(GET RESOURCES 0 _first_resource)
        cmake_path(GET _first_resource PARENT_PATH _resources_subdir)
        set(_resources_dir "${CMAKE_CURRENT_SOURCE_DIR}/${_resources_subdir}")
    else()
        set(_resources_dir "")
    endif()

    target_compile_definitions(${target} PUBLIC
        UI_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
        UI_MAIN="${UI_MAIN_FILE}"
        UI_RESOURCES_DIR="${_resources_dir}"
        QSJC_SYMBOL=qjsc_${_ui_stem}
        QSJC_SYMBOL_SIZE=qjsc_${_ui_stem}_size
        PACKAGE_NAME="${pkg_name}"
        SINGULARITY_WIDGETS_DIR="${SINGULARITY_ROOT_DIR}/widgets"
    )

    # Create a "singularity/" symlink in the binary dir so qjsc can resolve
    # "singularity/knob.js" imports relative to its working directory.
    # The source tree stays clean; the runtime normalizer in QuickJSEngine2.cpp
    # handles the same imports via the SINGULARITY_WIDGETS_DIR compile definition.
    file(CREATE_LINK
        "${SINGULARITY_ROOT_DIR}/widgets"
        "${CMAKE_CURRENT_BINARY_DIR}/singularity"
        SYMBOLIC
    )

    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated.h
        COMMAND $<TARGET_FILE:qjsc>
            -M singularity,js_init_module_singularity
            -o ${CMAKE_CURRENT_BINARY_DIR}/generated.h
            ${UI_MAIN_FILE}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS qjsc ${UI_MAIN_FILE}
        VERBATIM
    )

    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated_loader.h
        COMMAND ${CMAKE_COMMAND}
            -DGENERATED_H=${CMAKE_CURRENT_BINARY_DIR}/generated.h
            -DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/generated_loader.h
            -P ${SINGULARITY_ROOT_DIR}/cmake/generate_loader.cmake
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/generated.h
        VERBATIM
    )

    target_sources(${target} PRIVATE
        $<$<CONFIG:Release>:${CMAKE_CURRENT_BINARY_DIR}/generated_loader.h>
    )

    target_include_directories(${target} PRIVATE
        $<$<CONFIG:Release>:${CMAKE_CURRENT_BINARY_DIR}>
    )

    # --- Embed RESOURCES (images) as C arrays → APP only ---
    if(RESOURCES)
        set(_img_res_files "")
        set(_img_res_names "")
        foreach(_res ${RESOURCES})
            list(APPEND _img_res_files "${CMAKE_CURRENT_SOURCE_DIR}/${_res}")
            get_filename_component(_res_name "${_res}" NAME)
            list(APPEND _img_res_names "${_res_name}")
        endforeach()

        set(_generated_resources "${CMAKE_CURRENT_BINARY_DIR}/generated_resources.h")
        add_custom_command(
            OUTPUT "${_generated_resources}"
            COMMAND ${CMAKE_COMMAND}
                "-DRESOURCE_FILES=${_img_res_files}"
                "-DRESOURCE_NAMES=${_img_res_names}"
                "-DNAMESPACE=singularity_resources"
                "-DGENERATE_IMAGE_REGISTER=ON"
                "-DOUTPUT_FILE=${_generated_resources}"
                -P "${SINGULARITY_ROOT_DIR}/cmake/embed_resources.cmake"
            DEPENDS ${_img_res_files}
            VERBATIM
        )
    endif()

    # --- Embed DATA_RESOURCES as C arrays → both VST3 and APP ---
    if(DATA_RESOURCES)
        set(_data_res_files "")
        set(_data_res_names "")
        foreach(_res ${DATA_RESOURCES})
            list(APPEND _data_res_files "${CMAKE_CURRENT_SOURCE_DIR}/${_res}")
            get_filename_component(_res_name "${_res}" NAME)
            list(APPEND _data_res_names "${_res_name}")
        endforeach()

        set(_generated_data_resources "${CMAKE_CURRENT_BINARY_DIR}/generated_data_resources.h")
        add_custom_command(
            OUTPUT "${_generated_data_resources}"
            COMMAND ${CMAKE_COMMAND}
                "-DRESOURCE_FILES=${_data_res_files}"
                "-DRESOURCE_NAMES=${_data_res_names}"
                "-DNAMESPACE=singularity_data"
                "-DOUTPUT_FILE=${_generated_data_resources}"
                -P "${SINGULARITY_ROOT_DIR}/cmake/embed_resources.cmake"
            DEPENDS ${_data_res_files}
            VERBATIM
        )
    endif()

    foreach(type IN LISTS FORMATS)
        # Standalone plugin
        if(type STREQUAL "APP")
            add_executable(${target}_APP 
                ${SINGULARITY_ROOT_DIR}/standalone/main2.cpp
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
                set_target_properties(${target}_APP PROPERTIES MACOSX_BUNDLE TRUE)
            elseif(UNIX AND NOT APPLE)
                find_package(PkgConfig REQUIRED)
                pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)
                target_sources(${target}_APP PRIVATE ${SINGULARITY_ROOT_DIR}/standalone/PipeWire.cpp)
                target_include_directories(${target}_APP PRIVATE ${PIPEWIRE_INCLUDE_DIRS})
                target_link_libraries(${target}_APP PRIVATE ${PIPEWIRE_LIBRARIES})
            endif()

            target_include_directories(${target}_APP PRIVATE
                ${SINGULARITY_ROOT_DIR}/platform
                ${CMAKE_CURRENT_SOURCE_DIR}
            )
            target_compile_definitions(${target}_APP PRIVATE
                SINGULARITY_STANDALONE=1
                PLUGIN_CLASS=${PARAMS_PLUGIN_CLASS}
                PLUGIN_CLASS_HEADER="${PARAMS_PLUGIN_CLASS_HEADER}"
            )
            set_target_properties(${target}_APP PROPERTIES OUTPUT_NAME ${target})
            target_link_libraries(${target}_APP PRIVATE ${target})

            # APP uses embedded image resources
            if(RESOURCES)
                target_compile_definitions(${target}_APP PRIVATE SINGULARITY_HAS_EMBEDDED_RESOURCES=1)
                target_sources(${target}_APP PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated_resources.h)
                target_include_directories(${target}_APP PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
            endif()

            # APP uses embedded data resources
            if(DATA_RESOURCES)
                target_compile_definitions(${target}_APP PRIVATE SINGULARITY_HAS_EMBEDDED_DATA_RESOURCES=1)
                target_sources(${target}_APP PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated_data_resources.h)
                target_include_directories(${target}_APP PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
            endif()
            set_target_properties(${target}_APP PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/out/APP/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>)
        elseif(type STREQUAL "VST3")
            # Generate stable UIDs from plugin target name
            set(_uid_seed "${target}")

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
            set(SMTG_CUSTOM_BINARY_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/out)

            smtg_add_vst3plugin(${target}_VST3
                PACKAGE_NAME ${target}
                ${SINGULARITY_ROOT_DIR}/vst3/vst3version.h
                ${SINGULARITY_ROOT_DIR}/vst3/vst3processor.h
                ${SINGULARITY_ROOT_DIR}/vst3/vst3controller.h
                ${SINGULARITY_ROOT_DIR}/vst3/vst3controller.cpp
                ${SINGULARITY_ROOT_DIR}/vst3/vst3entry.cpp
                ${SINGULARITY_ROOT_DIR}/vst3/SingularityView.h
                ${SINGULARITY_ROOT_DIR}/vst3/SingularityView.cpp
            )

            if(RESOURCES)
                set(IMAGE_FILES)
                set(FONT_FILES)
                set(OTHER_FILES)
                foreach(RESOURCE IN LISTS RESOURCES)
                    get_filename_component(FILETYPE "${RESOURCE}" LAST_EXT)
                    if(FILETYPE MATCHES "^(.png|.jpg|.jpeg|.gif|.bmp|.webp|.tga|.tif|.tiff|.svg)$")
                        list(APPEND IMAGE_FILES "${RESOURCE}")
                    elseif(FILETYPE MATCHES "^(.ttf|.otf)$")
                        list(APPEND FONT_FILES "${RESOURCE}")
                    else()
                        list(APPEND OTHER_FILES "${RESOURCE}")
                    endif()
                endforeach()

                if(IMAGE_FILES)
                    smtg_target_add_plugin_resources(${target}_VST3 OUTPUT_SUBDIRECTORY "Images" RESOURCES ${IMAGE_FILES})
                endif()
                if(FONT_FILES)
                    smtg_target_add_plugin_resources(${target}_VST3 OUTPUT_SUBDIRECTORY "Fonts" RESOURCES ${FONT_FILES})
                endif()
                if(OTHER_FILES)
                    smtg_target_add_plugin_resources(${target}_VST3 RESOURCES ${OTHER_FILES})
                endif()
            endif()
            # VST3 uses embedded data resources
            if(DATA_RESOURCES)
                target_compile_definitions(${target}_VST3 PRIVATE SINGULARITY_HAS_EMBEDDED_DATA_RESOURCES=1)
                target_sources(${target}_VST3 PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated_data_resources.h)
            endif()

            target_compile_definitions(${target}_VST3 PRIVATE
                PLUGIN_NAME="${target}"
                VENDOR="${VENDOR}"
                URL="${URL}"
                EMAIL="${EMAIL}"
                PLUGIN_CLASS=${PARAMS_PLUGIN_CLASS}
                PLUGIN_CLASS_HEADER="${PARAMS_PLUGIN_CLASS_HEADER}"
            )

            target_link_libraries(${target}_VST3
                PRIVATE
                    sdk
                    ${target}
            )

            target_include_directories(${target}_VST3 PRIVATE
                ${SINGULARITY_ROOT_DIR}/platform
                ${SINGULARITY_ROOT_DIR}
                ${CMAKE_CURRENT_BINARY_DIR}
                ${CMAKE_CURRENT_SOURCE_DIR}
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