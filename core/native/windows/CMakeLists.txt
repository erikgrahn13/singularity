set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(windows WIN32
mainWin.cpp
${SINGULARITY_CORE_PATH}/Editor.cpp
EditorWin.cpp
${asiosdk_SOURCE_DIR}/host/pc/asiolist.cpp
${asiosdk_SOURCE_DIR}/host/asiodrivers.cpp
${asiosdk_SOURCE_DIR}/common/asio.cpp
)

target_include_directories(windows PRIVATE
    ${SINGULARITY_CORE_PATH}
    ${asiosdk_SOURCE_DIR}/common
    ${asiosdk_SOURCE_DIR}/host
    ${asiosdk_SOURCE_DIR}/host/pc
    ${skia_headers_SOURCE_DIR}
)

#find_package(unofficial-skia CONFIG REQUIRED)
target_link_libraries(windows PRIVATE ${skia_SOURCE_DIR}/${SKIA_LIB})

# Configure runtime library options for Debug and Release builds
if(MSVC)
    # Force /MD for both Debug and Release builds to match Skia's release runtime
    target_compile_options(windows PRIVATE
        $<$<CONFIG:Debug>:/MD>   # Use /MD (Release runtime) in Debug build
        $<$<CONFIG:Release>:/MD> # Use /MD (Release runtime) in Release build
    )

    # Disable _ITERATOR_DEBUG_LEVEL for Debug builds, as Skia was built in Release mode
    add_definitions(-D_ITERATOR_DEBUG_LEVEL=0)
endif()
