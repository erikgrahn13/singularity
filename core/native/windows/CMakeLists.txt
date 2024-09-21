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
)

find_package(unofficial-skia CONFIG REQUIRED)
target_link_libraries(windows PRIVATE unofficial::skia::skia)