cmake_minimum_required(VERSION 3.20)

project(ExamplePlugin
        VERSION 1.0.0.0
        DESCRIPTION "This is my example plugin"
)

add_subdirectory(../ ${CMAKE_BINARY_DIR}/singularity)

singularity_create_plugin(
    ${PROJECT_NAME}
    SOURCES
        Example.cpp
    UI
        bloom.js
    PLUGIN_WIDTH 400
    PLUGIN_HEIGHT 300
    FORMATS APP VST3
)

