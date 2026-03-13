cmake_minimum_required(VERSION 3.20)

project(Singularity LANGUAGES Swift CXX)

include(FetchContent)
if(WIN32)
    set(SKIA_URL https://github.com/olilarkin/skia-builder/releases/download/chrome%2Fm144/skia-build-win-x64-gpu-release.zip)
else()
    set(SKIA_URL https://github.com/olilarkin/skia-builder/releases/download/chrome%2Fm144/skia-build-mac-universal-gpu-release.zip)
endif()

FetchContent_Declare(
    skia
    URL ${SKIA_URL}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(skia)

if(WIN32)
    add_executable(main WIN32 windows/main.cpp SingularityGraphics.cpp)
    target_compile_definitions(main PRIVATE NOMINMAX)
    set_property(TARGET main PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
    target_link_libraries(main PRIVATE ${skia_SOURCE_DIR}/win-gpu/lib/Release/x64/skia.lib)
else()
    add_executable(main AppDelegate.swift)
    target_link_libraries(main PRIVATE ${skia_SOURCE_DIR}/mac-gpu/lib/Release/libskia.a)
    target_compile_options(main PRIVATE
        $<$<COMPILE_LANGUAGE:Swift>:-cxx-interoperability-mode=default>
    )
    
    set_target_properties(main PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/Info.plist
    )
endif()

target_compile_features(main PRIVATE cxx_std_23)
target_include_directories(main PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${skia_SOURCE_DIR}/include
)
