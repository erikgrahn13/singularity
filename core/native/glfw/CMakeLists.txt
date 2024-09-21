add_executable(mainglfw main.cpp)

target_link_libraries(mainglfw ${skia_SOURCE_DIR}/${SKIA_LIB}
    ${glfw_SOURCE_DIR}/${GLFW_LIB}
)

target_include_directories(mainglfw PRIVATE ${glfw_SOURCE_DIR}/include)
add_dependencies(mainglfw skia_headers)


