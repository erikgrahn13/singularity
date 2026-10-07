cmake_minimum_required(VERSION 3.20)

project(ExampleEffect
        VERSION 1.0.0.0
        DESCRIPTION "This is my example effect plugin"
)

singularity_create_plugin(
    ${PROJECT_NAME}
    PLUGIN_NAME "Example Effect"
    FORMATS APP VST3
    PLUGIN_CLASS ExampleEffect
    PLUGIN_CLASS_HEADER "ExampleEffect.h"
    QML_FILES
        DragAndDrop.qml
        example.mjs
        Waveform.qml
    RESOURCES
        resources/logo_transparent.png
    SHADERS
        fairy_smoke.frag
)
