add_executable(mainglfw main.cpp)

target_link_libraries(mainglfw ${skia_SOURCE_DIR}/${SKIA_LIB}
    ${glfw_SOURCE_DIR}/${GLFW_LIB}
)

target_include_directories(mainglfw PRIVATE 
    ${glfw_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/external/skia-headers)
add_dependencies(mainglfw skia_headers)

if(APPLE)
    target_link_libraries(mainglfw
        "-framework Cocoa"
        "-framework CoreFoundation"
        "-framework IOKit"
        "-framework CoreVideo"
        "-framework AppKit"
        "-framework Foundation"
    )
endif()

