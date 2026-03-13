cmake_minimum_required(VERSION 3.20)

project(Singularity)

if(WIN32)
include(FetchContent)

FetchContent_Declare(
    skia
    URL https://github.com/olilarkin/skia-builder/releases/download/chrome%2Fm144/skia-build-win-x64-gpu-release.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )

        
        FetchContent_MakeAvailable(skia)
        
endif()

add_executable(main WIN32 windows/main.cpp SingularityGraphics.cpp)
target_compile_features(main PRIVATE cxx_std_23)
target_include_directories(main PRIVATE ${skia_SOURCE_DIR}/include)
target_compile_definitions(main PRIVATE NOMINMAX)
set_property(TARGET main PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
target_link_libraries(main PRIVATE ${skia_SOURCE_DIR}/win-gpu/lib/Release/x64/skia.lib)

