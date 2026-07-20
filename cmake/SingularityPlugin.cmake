include_guard(GLOBAL)
cmake_minimum_required(VERSION 3.22)

cmake_path(GET CMAKE_CURRENT_LIST_DIR PARENT_PATH _singularity_root_dir)
set(SINGULARITY_ROOT_DIR "${_singularity_root_dir}" CACHE INTERNAL "" FORCE)
set(SINGULARITY_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE INTERNAL "" FORCE)

function(_singularity_enable_desktop_support)
    get_property(_desktop_support_enabled GLOBAL PROPERTY SINGULARITY_DESKTOP_SUPPORT_ENABLED)
    if(_desktop_support_enabled)
        return()
    endif()

    # Must be set before dependency and plug-in targets are created. Skia uses
    # the release static runtime on Windows, and VST3 shared objects need PIC.
    if(WIN32)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded" CACHE STRING "" FORCE)
    elseif(UNIX AND NOT APPLE)
        set(CMAKE_POSITION_INDEPENDENT_CODE ON)
        find_package(X11 REQUIRED)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(LIBPORTAL REQUIRED libportal)
        set_target_properties(X11::X11 PROPERTIES IMPORTED_GLOBAL TRUE)
        set_target_properties(X11::Xrandr PROPERTIES IMPORTED_GLOBAL TRUE)
        set(SINGULARITY_LIBPORTAL_LIBRARIES "${LIBPORTAL_LIBRARIES}" CACHE INTERNAL "")
        set(SINGULARITY_LIBPORTAL_INCLUDE_DIRS "${LIBPORTAL_INCLUDE_DIRS}" CACHE INTERNAL "")
    endif()

    list(APPEND CMAKE_MODULE_PATH "${SINGULARITY_CMAKE_DIR}")
    include(FetchContent)
    include(DMON)
    include(SKIA)
    include(CHOC)
    include(QUICKJS)

    set(SINGULARITY_SKIA_INCLUDE_DIR "${skia_SOURCE_DIR}/include" CACHE INTERNAL "")
    set(SINGULARITY_QUICKJS_DIR "${quickjs_SOURCE_DIR}" CACHE INTERNAL "")
    set(SINGULARITY_SKIA_LIB "${SKIA_LIB}" CACHE INTERNAL "")
    set_property(GLOBAL PROPERTY SINGULARITY_DESKTOP_SUPPORT_ENABLED TRUE)
endfunction()


function(singularity_create_plugin target)
    set(oneValueArgs
        VENDOR BUNDLE_ID URL EMAIL PLUGIN_CLASS PLUGIN_CLASS_HEADER PLUGIN_NAME
        CAPI_SDK_DIR CAPI_MAX_BLOCK_SIZE CAPI_STACK_SIZE)
    set(multiValueArgs SOURCES UI FORMATS RESOURCES DATA_RESOURCES CAPI_INCLUDE_DIRS)

    # Parse the arguments
    cmake_parse_arguments(PARAMS "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

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

    if(NOT SOURCES)
        set(SOURCES ${PARAMS_UNPARSED_ARGUMENTS})
    endif()

    list(FIND FORMATS "CAPI" _capi_format_index)
    list(FIND FORMATS "APP" _app_format_index)
    list(FIND FORMATS "VST3" _vst3_format_index)

    if(NOT _app_format_index EQUAL -1 OR NOT _vst3_format_index EQUAL -1)
        _singularity_enable_desktop_support()
    endif()

    if(NOT _capi_format_index EQUAL -1)
        include("${SINGULARITY_ROOT_DIR}/capi/SingularityCapi.cmake")
        singularity_create_capi_plugin(${target}
            PLUGIN_CLASS "${PARAMS_PLUGIN_CLASS}"
            PLUGIN_CLASS_HEADER "${PARAMS_PLUGIN_CLASS_HEADER}"
            SDK_DIR "${PARAMS_CAPI_SDK_DIR}"
            MAX_BLOCK_SIZE "${PARAMS_CAPI_MAX_BLOCK_SIZE}"
            STACK_SIZE "${PARAMS_CAPI_STACK_SIZE}"
            SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
            BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}"
            SOURCES ${SOURCES}
            INCLUDE_DIRS ${PARAMS_CAPI_INCLUDE_DIRS}
        )
    endif()

    # CAPI is headless. A CAPI-only plug-in deliberately skips the UI library,
    # App.js validation, qjsc generation, Skia, QuickJS, and platform windows.
    if(_app_format_index EQUAL -1 AND _vst3_format_index EQUAL -1)
        if(_capi_format_index EQUAL -1)
            message(FATAL_ERROR "[singularity] Target '${target}' has no supported FORMATS.")
        endif()
        add_library(${target} ALIAS ${target}_CAPI)
        return()
    endif()

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

    add_library(${target} STATIC
        ${SOURCES}
        ${SINGULARITY_ROOT_DIR}/SingularityController.cpp
        ${SINGULARITY_ROOT_DIR}/chocFileWatcher.cpp
        ${SINGULARITY_ROOT_DIR}/QuickJSEngine.cpp
        ${SINGULARITY_ROOT_DIR}/SkiaRenderer.cpp
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
        ${SINGULARITY_ROOT_DIR}
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
	target_link_libraries(${target} PUBLIC dxguid dxgi d3d12 OneCore)
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
        set_target_properties(${target} PROPERTIES POSITION_INDEPENDENT_CODE ON)
        target_sources(${target} PRIVATE ${SINGULARITY_ROOT_DIR}/platform/linux/X11Window.cpp)
        target_link_libraries(${target} PUBLIC
            X11::X11
            X11::Xrandr
            ${SINGULARITY_LIBPORTAL_LIBRARIES}
        )
        target_include_directories(${target} PRIVATE ${SINGULARITY_LIBPORTAL_INCLUDE_DIRS})
    endif()


    # Resolve the UI entry point — defaults to App.js if UI was not specified.
    if(PARAMS_UI)
        list(GET PARAMS_UI 0 UI_MAIN_FILE)
    else()
        set(UI_MAIN_FILE "App.js")
    endif()
    if(NOT IS_ABSOLUTE "${UI_MAIN_FILE}")
        set(UI_MAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${UI_MAIN_FILE}")
    endif()
    if(NOT EXISTS "${UI_MAIN_FILE}")
        message(FATAL_ERROR "[singularity] No App.js found at '${UI_MAIN_FILE}'. "
            "Each plugin must provide an App.js (or pass UI <file> explicitly).")
    endif()

    # If the entry point is App.js, plugin authors don't need a boilerplate main.js:
    # - Debug: QuickJSEngine2.cpp detects App.js and synthesizes the mount wrapper in-memory.
    # - Release: qjsc needs a real file to compile, so we generate a wrapper main.js in
    #   the build dir for that step only. UI_MAIN still points to App.js so the debug
    #   path gets the original file.
    cmake_path(GET UI_MAIN_FILE FILENAME _ui_main_filename)
    if(_ui_main_filename STREQUAL "App.js")
        set(_qjsc_input "${CMAKE_CURRENT_BINARY_DIR}/main.js")
        file(WRITE "${_qjsc_input}"
            "import { mount } from \"singularity\";\n"
            "import App from \"${UI_MAIN_FILE}\";\n"
            "mount(App);\n"
        )
        set(_ui_stem "main")
    else()
        set(_qjsc_input "${UI_MAIN_FILE}")
        cmake_path(GET UI_MAIN_FILE STEM _ui_stem)
    endif()

    if(RESOURCES)
        list(GET RESOURCES 0 _first_resource)
        cmake_path(GET _first_resource PARENT_PATH _resources_subdir)
        set(_resources_dir "${CMAKE_CURRENT_SOURCE_DIR}/${_resources_subdir}")
    else()
        set(_resources_dir "")
    endif()

    if(PARAMS_PLUGIN_NAME)
        set(_plugin_title "${PARAMS_PLUGIN_NAME}")
    else()
        set(_plugin_title "${target}")
    endif()

    target_compile_definitions(${target} PUBLIC
        UI_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
        UI_MAIN="${UI_MAIN_FILE}"
        UI_RESOURCES_DIR="${_resources_dir}"
        QSJC_SYMBOL=qjsc_${_ui_stem}
        QSJC_SYMBOL_SIZE=qjsc_${_ui_stem}_size
        UI_MAIN_IS_APP_JS=$<IF:$<STREQUAL:${_ui_main_filename},App.js>,1,0>
        SINGULARITY_WIDGETS_DIR="${SINGULARITY_ROOT_DIR}/widgets"
        PLUGIN_NAME="${_plugin_title}"
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

    # Imported widgets are compiled into the release UI by qjsc. Track all widget
    # sources so editing App.js or a widget cannot leave stale embedded bytecode.
    file(GLOB _singularity_widget_sources CONFIGURE_DEPENDS
        "${SINGULARITY_ROOT_DIR}/widgets/*.js"
    )

    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated.h
        COMMAND $<TARGET_FILE:qjsc>
            -M singularity,js_init_module_singularity
            -o ${CMAKE_CURRENT_BINARY_DIR}/generated.h
            ${_qjsc_input}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS qjsc ${_qjsc_input} "${UI_MAIN_FILE}" ${_singularity_widget_sources}
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

    if(NOT _app_format_index EQUAL -1)
        include("${SINGULARITY_ROOT_DIR}/standalone/SingularityApp.cmake")
        singularity_create_app_plugin(${target}
            PLUGIN_CLASS "${PARAMS_PLUGIN_CLASS}"
            PLUGIN_CLASS_HEADER "${PARAMS_PLUGIN_CLASS_HEADER}"
            BASE_TARGET "${target}"
            SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
            BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}"
            GENERATED_RESOURCES "${_generated_resources}"
            GENERATED_DATA_RESOURCES "${_generated_data_resources}"
            RESOURCES ${RESOURCES}
            DATA_RESOURCES ${DATA_RESOURCES}
        )
    endif()

    if(NOT _vst3_format_index EQUAL -1)
        include("${SINGULARITY_ROOT_DIR}/vst3/SingularityVst3.cmake")
        singularity_create_vst3_plugin(${target}
            PLUGIN_TITLE "${_plugin_title}"
            VENDOR "${VENDOR}"
            URL "${URL}"
            EMAIL "${EMAIL}"
            BUNDLE_ID "${BUNDLE_ID}"
            PLUGIN_CLASS "${PARAMS_PLUGIN_CLASS}"
            PLUGIN_CLASS_HEADER "${PARAMS_PLUGIN_CLASS_HEADER}"
            BASE_TARGET "${target}"
            SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
            BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}"
            GENERATED_DATA_RESOURCES "${_generated_data_resources}"
            RESOURCES ${RESOURCES}
            DATA_RESOURCES ${DATA_RESOURCES}
        )
    endif()


endfunction()
