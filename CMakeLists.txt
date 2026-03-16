cmake_minimum_required(VERSION 3.20)

project(Singularity LANGUAGES CXX)

include(FetchContent)
if(WIN32)
    set(SKIA_URL https://github.com/olilarkin/skia-builder/releases/download/chrome%2Fm144/skia-build-win-x64-gpu-release.zip)
else()
    set(SKIA_URL https://github.com/olilarkin/skia-builder/releases/download/chrome%2Fm144/skia-build-mac-universal-gpu-release.zip)
endif()

FetchContent_Declare(
    dmon
    GIT_REPOSITORY https://github.com/septag/dmon.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)

FetchContent_Populate(dmon)

FetchContent_Declare(
    skia
    URL ${SKIA_URL}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(skia)

FetchContent_Declare(
    quickjs
    URL https://github.com/quickjs-ng/quickjs/archive/refs/tags/v0.12.1.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(quickjs)


if(WIN32)
    add_executable(main WIN32 windows/main.cpp SingularityGraphics.cpp)
    target_compile_definitions(main PRIVATE NOMINMAX)
    set_property(TARGET main PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
    set_property(TARGET qjs PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
    set_property(TARGET qjs-libc PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
    target_link_libraries(main PRIVATE ${skia_SOURCE_DIR}/win-gpu/lib/Release/x64/skia.lib
                                       dwrite.lib)  # DirectWrite — required by Skia's DirectWrite font manager
else()
    enable_language(Swift)
    add_executable(main macos/AppDelegate.swift)
    target_link_libraries(main PRIVATE ${skia_SOURCE_DIR}/mac-gpu/lib/Release/libskia.a)
    target_compile_options(main PRIVATE
        $<$<COMPILE_LANGUAGE:Swift>:-cxx-interoperability-mode=default>
    )
    
    set_target_properties(main PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/macos/Info.plist
    )
endif()

target_sources(main PRIVATE 
    SingularityGraphics2.cpp
    dmonFileWatcher.cpp
    SkiaRenderer.cpp
    QuickJSEngine.cpp
)

target_compile_definitions(main PRIVATE
    JS_SCRIPTS_DIR="${CMAKE_SOURCE_DIR}"
)

target_link_libraries(main PRIVATE qjs-libc)
target_compile_features(main PRIVATE cxx_std_23)
target_include_directories(main PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${skia_SOURCE_DIR}/include
    ${quickjs_SOURCE_DIR}
    ${dmon_SOURCE_DIR}
)
