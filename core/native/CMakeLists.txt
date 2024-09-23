# Locate FreeType
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
find_package(Freetype REQUIRED)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
find_library(COREFOUNDATION_FRAMEWORK CoreFoundation)
find_library(IOKIT_FRAMEWORK IOKit)
endif()
add_executable(sdl_window main.cpp)

# Include directories for FreeType and Skia
target_include_directories(sdl_window PRIVATE ${FREETYPE_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/external/skia-headers)

# Link FreeType, SDL, and Skia libraries
target_link_libraries(sdl_window PRIVATE SDL2::SDL2-static SDL2::SDL2main ${skia_SOURCE_DIR}/${SKIA_LIB} ${COREFOUNDATION_FRAMEWORK} ${IOKIT_FRAMEWORK})

# Add Skia as a dependency
add_dependencies(sdl_window skia_headers)
