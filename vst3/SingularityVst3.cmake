include_guard(GLOBAL)

# Options must be set before FetchContent_MakeAvailable so the VST3 SDK
# CMakeLists.txt sees them when FetchContent calls add_subdirectory.
option(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES "Enable VST 3 Plug-in Examples" OFF)
option(SMTG_ENABLE_VST3_HOSTING_EXAMPLES "Enable VST 3 Hosting Examples" OFF)
option(SMTG_ENABLE_VSTGUI_SUPPORT "Enable VSTGUI Support" OFF)
option(JS_HOT_RELOAD "Watch and hot-reload JS scripts at runtime" ON)
option(SMTG_USE_STATIC_CRT "use static CRuntime on Windows (option /MT)" ON)

FetchContent_Declare(
    vst3sdk
    GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk
    GIT_TAG v3.8.0_build_66
    GIT_SHALLOW TRUE
    GIT_SUBMODULES "base" "cmake" "pluginterfaces" "public.sdk"
    GIT_SUBMODULES_RECURSE FALSE
    GIT_SUBMODULES_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(vst3sdk)

smtg_enable_vst3_sdk()

# smtg_configure_cmake_generator() sets CMAKE_CONFIGURATION_TYPES in its own
# directory scope. Promote it so consumers using Singularity via FetchContent
# can also create correctly structured Windows VST3 bundles.
if(WIN32)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Build types" FORCE)
endif()

# The SDK's threadchecker_mac.mm uses std::terminate() without including
# <exception>. Newer Xcode/macOS SDKs no longer provide it transitively.
if(APPLE AND TARGET sdk_common)
    target_compile_options(sdk_common PRIVATE "-include" "exception")
endif()

set(SINGULARITY_VST3SDK_SOURCE_DIR "${vst3sdk_SOURCE_DIR}" CACHE INTERNAL "" FORCE)
set(SINGULARITY_VST3_PUBLIC_SDK_DIR "${vst3sdk_SOURCE_DIR}/public.sdk" CACHE INTERNAL "" FORCE)

if(WIN32 AND MSVC)
    # SMTG_PlatformToolset adds /MTd in Debug. Override it to match the
    # prebuilt Skia library, which always uses the static release runtime.
    foreach(_target IN ITEMS
            sdk base pluginterfaces sdk_common sdk_hosting moduleinfotool
            validator editorhost audiohost)
        if(TARGET ${_target})
            target_compile_options(${_target} PRIVATE $<$<CONFIG:Debug>:/MT>)
        endif()
    endforeach()
    add_compile_options($<$<CONFIG:Debug>:/MT>)
endif()

function(singularity_create_vst3_plugin target)
    set(oneValueArgs
        PLUGIN_TITLE
        VENDOR
        URL
        EMAIL
        BUNDLE_ID
        PLUGIN_CLASS
        PLUGIN_CLASS_HEADER
        BASE_TARGET
        SOURCE_DIR
        BINARY_DIR
        GENERATED_DATA_RESOURCES)
    set(multiValueArgs RESOURCES DATA_RESOURCES)
    cmake_parse_arguments(VST3 "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Derive stable processor and controller UIDs from the public target name.
    string(MD5 _processor_hash "${target}_processor")
    string(MD5 _controller_hash "${target}_controller")

    string(SUBSTRING "${_processor_hash}" 0  8 PROC_UID_0)
    string(SUBSTRING "${_processor_hash}" 8  8 PROC_UID_1)
    string(SUBSTRING "${_processor_hash}" 16 8 PROC_UID_2)
    string(SUBSTRING "${_processor_hash}" 24 8 PROC_UID_3)
    string(SUBSTRING "${_controller_hash}" 0  8 CTRL_UID_0)
    string(SUBSTRING "${_controller_hash}" 8  8 CTRL_UID_1)
    string(SUBSTRING "${_controller_hash}" 16 8 CTRL_UID_2)
    string(SUBSTRING "${_controller_hash}" 24 8 CTRL_UID_3)

    foreach(_uid_part IN ITEMS
            PROC_UID_0 PROC_UID_1 PROC_UID_2 PROC_UID_3
            CTRL_UID_0 CTRL_UID_1 CTRL_UID_2 CTRL_UID_3)
        string(TOUPPER "${${_uid_part}}" ${_uid_part})
    endforeach()

    configure_file(
        "${SINGULARITY_ROOT_DIR}/vst3/vst3plugincids.h.in"
        "${VST3_BINARY_DIR}/plugincids.h"
    )

    set(public_sdk_SOURCE_DIR ${SINGULARITY_VST3_PUBLIC_SDK_DIR})
    set(SMTG_CUSTOM_BINARY_LOCATION ${VST3_BINARY_DIR}/out)

    smtg_add_vst3plugin(${target}_VST3
        PACKAGE_NAME "${VST3_PLUGIN_TITLE}"
        ${SINGULARITY_ROOT_DIR}/vst3/vst3version.h
        ${SINGULARITY_ROOT_DIR}/vst3/vst3processor.h
        ${SINGULARITY_ROOT_DIR}/vst3/vst3controller.h
        ${SINGULARITY_ROOT_DIR}/vst3/vst3controller.cpp
        ${SINGULARITY_ROOT_DIR}/vst3/vst3entry.cpp
        ${SINGULARITY_ROOT_DIR}/vst3/SingularityView.h
        ${SINGULARITY_ROOT_DIR}/vst3/SingularityView.cpp
    )

    if(VST3_RESOURCES)
        set(_image_files)
        set(_font_files)
        set(_other_files)
        foreach(_resource IN LISTS VST3_RESOURCES)
            get_filename_component(_file_type "${_resource}" LAST_EXT)
            if(_file_type MATCHES "^(.png|.jpg|.jpeg|.gif|.bmp|.webp|.tga|.tif|.tiff|.svg)$")
                list(APPEND _image_files "${_resource}")
            elseif(_file_type MATCHES "^(.ttf|.otf)$")
                list(APPEND _font_files "${_resource}")
            else()
                list(APPEND _other_files "${_resource}")
            endif()
        endforeach()

        if(_image_files)
            smtg_target_add_plugin_resources(${target}_VST3
                OUTPUT_SUBDIRECTORY "Images"
                RESOURCES ${_image_files})
        endif()
        if(_font_files)
            smtg_target_add_plugin_resources(${target}_VST3
                OUTPUT_SUBDIRECTORY "Fonts"
                RESOURCES ${_font_files})
        endif()
        if(_other_files)
            smtg_target_add_plugin_resources(${target}_VST3 RESOURCES ${_other_files})
        endif()
    endif()

    if(VST3_DATA_RESOURCES)
        target_compile_definitions(${target}_VST3 PRIVATE
            SINGULARITY_HAS_EMBEDDED_DATA_RESOURCES=1)
        target_sources(${target}_VST3 PRIVATE "${VST3_GENERATED_DATA_RESOURCES}")
    endif()

    target_compile_definitions(${target}_VST3 PRIVATE
        VENDOR="${VST3_VENDOR}"
        URL="${VST3_URL}"
        EMAIL="${VST3_EMAIL}"
        PLUGIN_CLASS=${VST3_PLUGIN_CLASS}
        PLUGIN_CLASS_HEADER="${VST3_PLUGIN_CLASS_HEADER}"
    )

    target_link_libraries(${target}_VST3 PRIVATE sdk ${VST3_BASE_TARGET})

    target_include_directories(${target}_VST3 PRIVATE
        ${SINGULARITY_ROOT_DIR}/platform
        ${SINGULARITY_ROOT_DIR}
        ${VST3_BINARY_DIR}
        ${VST3_SOURCE_DIR}
    )

    smtg_target_configure_version_file(${target}_VST3)

    if(SMTG_MAC)
        smtg_target_set_bundle(${target}_VST3
            BUNDLE_IDENTIFIER ${VST3_BUNDLE_ID}
            COMPANY_NAME ${VST3_VENDOR}
        )
    elseif(SMTG_WIN)
        target_sources(${target}_VST3 PRIVATE
            ${SINGULARITY_ROOT_DIR}/vst3/win32resource.rc
        )
    endif()
endfunction()
