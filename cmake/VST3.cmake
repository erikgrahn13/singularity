FetchContent_Declare(
    vst3sdk
    GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk
    GIT_TAG v3.8.0_build_66
    GIT_SHALLOW TRUE
    GIT_SUBMODULES "base" "cmake" "pluginterfaces" "public.sdk"
    GIT_SUBMODULES_RECURSE FALSE
    GIT_SUBMODULES_SHALLOW TRUE
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/vst3sdk
    SOURCE_SUBDIR IGNORE
)

FetchContent_MakeAvailable(vst3sdk)

set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "" FORCE)

option(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES "Enable VST 3 Plug-in Examples" OFF)
option(SMTG_ENABLE_VST3_HOSTING_EXAMPLES "Enable VST 3 Hosting Examples" OFF)
option(SMTG_ENABLE_VSTGUI_SUPPORT "Enable VSTGUI Support" OFF)
option(JS_HOT_RELOAD "Watch and hot-reload JS scripts at runtime" ON)
option(SMTG_USE_STATIC_CRT "use static CRuntime on Windows (option /MT)" ON)

add_subdirectory(${vst3sdk_SOURCE_DIR})
smtg_enable_vst3_sdk()

if(WIN32 AND MSVC)
    # SMTG_PlatformToolset adds /MTd for Debug to all vst3sdk targets.
    # Override to /MT to match prebuilt Skia (always /MT, never /MTd).
    foreach(_tgt IN ITEMS sdk base pluginterfaces sdk_common sdk_hosting moduleinfotool validator editorhost audiohost)
        if(TARGET ${_tgt})
            target_compile_options(${_tgt} PRIVATE $<$<CONFIG:Debug>:/MT>)
        endif()
    endforeach()
    add_compile_options($<$<CONFIG:Debug>:/MT>)
endif()
