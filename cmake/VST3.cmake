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

# FetchContent_MakeAvailable calls add_subdirectory exactly once — no explicit call needed.
FetchContent_MakeAvailable(vst3sdk)

smtg_enable_vst3_sdk()

# smtg_configure_cmake_generator() (called above) sets CMAKE_CONFIGURATION_TYPES
# via a plain set(), which is scoped to this directory.  When Singularity is
# consumed as a FetchContent dependency that value never reaches the consumer's
# root scope, so the VST3 SDK's Windows-bundle foreach loop sees an empty list
# and fails to redirect the DLL into Contents/<arch>/ → LNK1104.
# Using CACHE FORCE promotes the value to the global CMake cache, making it
# visible to every target in the build regardless of scope.
if(WIN32)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Build types" FORCE)
endif()

# Workaround: VST3 SDK threadchecker_mac.mm uses std::terminate() without
# #include <exception>. Newer Xcode/macOS SDKs no longer transitively provide it.
if(APPLE AND TARGET sdk_common)
    target_compile_options(sdk_common PRIVATE "-include" "exception")
endif()

# Export resolved VST3 SDK paths for consumers (e.g. SingularityPlugin.cmake)
set(SINGULARITY_VST3SDK_SOURCE_DIR "${vst3sdk_SOURCE_DIR}" CACHE INTERNAL "" FORCE)
set(SINGULARITY_VST3_PUBLIC_SDK_DIR "${vst3sdk_SOURCE_DIR}/public.sdk" CACHE INTERNAL "" FORCE)

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
