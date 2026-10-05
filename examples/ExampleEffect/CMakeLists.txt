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
        BloomDemo.qml
        DragAndDrop.qml
        example.mjs
    RESOURCES
        resources/logo_transparent.png
    SHADERS
        bloom_blur.frag
        bloom_composite.frag
        bloom_source.frag
        bloom_threshold.frag
        test.frag
    #     resources/mb-forever-raw.regular.ttf
    # DATA_RESOURCES
    #     data/logo_transparent2.png

)

# singularity_configure_vst3(
#     ${PROJECT_NAME}
#     SUBCATEGORIES Tools
#     SNAPSHOT resources/logo_transparent.png
# )
