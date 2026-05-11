cmake_minimum_required(VERSION 3.22)

project(ExamplePlugin VERSION 0.0.1)

singularity_create_plugin(${PROJECT_NAME}
    SOURCES
    ExampleController.h
    ExampleController.cpp
    ExampleProcessor.h
    ExampleProcessor.cpp

    FORMATS Standalone VST3
    PLUGIN_WIDTH 800
    PLUGIN_HEIGHT 600
)
