find_package(X11 REQUIRED)
find_package(Freetype REQUIRED)

# Add the executable
add_executable(linux_window main.cpp)

# Link SFML libraries to the executable
target_link_libraries(linux_window  ${X11_LIBRARIES} ${skia_SOURCE_DIR}/${SKIA_LIB} ${FREETYPE_LIBRARIES})
target_include_directories(linux_window PRIVATE ${X11_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/external/skia_headers)

#add_dependencies(linux_window skia_headers)
