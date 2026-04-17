if(WIN32)
    set(SKIA_URL https://github.com/olilarkin/skia-builder/releases/download/chrome%2Fm144/skia-build-win-x64-gpu-release.zip)
elseif(APPLE)
    set(SKIA_URL https://github.com/olilarkin/skia-builder/releases/download/chrome%2Fm144/skia-build-mac-universal-gpu-release.zip)
else()
    set(SKIA_URL https://github.com/olilarkin/skia-builder/releases/download/chrome%2Fm144/skia-build-linux-x64-gpu-release.zip)
endif()

FetchContent_Declare(
    skia
    URL ${SKIA_URL}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/skia
)

FetchContent_MakeAvailable(skia)

if(WIN32)
    set(SKIA_LIB ${skia_SOURCE_DIR})
elseif(APPLE)
    set(SKIA_LIB ${skia_SOURCE_DIR}/mac-gpu/lib/Release/libskia.a)
else()
    set(SKIA_URL https://github.com/olilarkin/skia-builder/releases/download/chrome%2Fm144/skia-build-linux-x64-gpu-release.zip)
endif()
