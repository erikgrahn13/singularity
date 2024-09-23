# Locate FreeType
# find_package(Freetype REQUIRED)

# add_executable(sdl_window main.cpp)

# # Include directories for FreeType and Skia
# target_include_directories(sdl_window PRIVATE ${FREETYPE_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/external/skia-headers)

# # Link FreeType, SDL, and Skia libraries
# target_link_libraries(sdl_window PRIVATE SDL2::SDL2-static SDL2::SDL2main ${skia_SOURCE_DIR}/${SKIA_LIB} ${FREETYPE_LIBRARIES})

# # Add Skia as a dependency
# add_dependencies(sdl_window skia_headers)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_subdirectory(windows)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_subdirectory(linux)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    add_subdirectory(macos)
endif()
