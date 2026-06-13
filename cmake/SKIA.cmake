if(WIN32)
    # set(SKIA_URL https://github.com/olilarkin/skia-builder/releases/download/chrome%2Fm144/skia-build-win-x64-gpu-release.zip)
    set(SKIA_URL https://github.com/erikgrahn13/skia-builder/releases/download/chrome%2Fm144/skia-build-win-x64-gpu-release.zip)
elseif(APPLE)
    set(SKIA_URL https://github.com/erikgrahn13/skia-builder/releases/download/chrome%2Fm144/skia-build-mac-universal-gpu-release.zip)
else()
    set(SKIA_URL https://github.com/erikgrahn13/skia-builder/releases/download/chrome%2Fm144/skia-build-linux-x64-gpu-release.zip)
endif()

FetchContent_Declare(
    skia
    URL ${SKIA_URL}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(skia)

if(WIN32)
    set(SKIA_LIB
        ${skia_SOURCE_DIR}/win-gpu/lib/Release/x64/skia.lib
        ${skia_SOURCE_DIR}/win-gpu/lib/Release/x64/dawn_combined.lib)
elseif(APPLE)
    set(SKIA_LIB
        ${skia_SOURCE_DIR}/mac-gpu/lib/Release/libskia.a
        ${skia_SOURCE_DIR}/mac-gpu/lib/Release/libdawn_combined.a)
else()
    set(SKIA_LIB
        ${skia_SOURCE_DIR}/linux-gpu/lib/Release/x64/libskia.a
        ${skia_SOURCE_DIR}/linux-gpu/lib/Release/x64/libdawn_combined.a)
endif()
