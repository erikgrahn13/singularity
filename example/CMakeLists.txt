cmake_minimum_required(VERSION 3.20)

project(ExamplePlugin
        VERSION 1.0.0.0
        DESCRIPTION "This is my example plugin"
)

singularity_create_plugin(
    ${PROJECT_NAME}
    SOURCES
        Example.cpp
    UI
        main.js
    FORMATS APP VST3
)

