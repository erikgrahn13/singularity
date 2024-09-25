find_package(X11 REQUIRED)
find_package(Freetype REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FONTCONFIG REQUIRED fontconfig)

# Add the executable
add_executable(linux_window 
    main.cpp
    ${SINGULARITY_CORE_PATH}/Editor.cpp
    EditorLinux.cpp
    )

# Link SFML libraries to the executable
target_link_libraries(linux_window  ${X11_LIBRARIES} ${skia_SOURCE_DIR}/${SKIA_LIB} ${FREETYPE_LIBRARIES} ${FONTCONFIG_LIBRARIES})
target_include_directories(linux_window PRIVATE $
    {X11_INCLUDE_DIR} 
    ${CMAKE_SOURCE_DIR}/external/skia_headers
    ${SINGULARITY_CORE_PATH}
    ${FONTCONFIG_INCLUDE_DIRS}
)
