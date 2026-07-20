cmake_minimum_required(VERSION 3.20)

project(ExampleEffect
        VERSION 1.0.0.0
        DESCRIPTION "This is my example effect plugin"
)

if(SINGULARITY_CAPI_SDK_DIR)
    set(EXAMPLE_EFFECT_FORMATS CAPI)
else()
    set(EXAMPLE_EFFECT_FORMATS APP VST3)
endif()

singularity_create_plugin(
    ${PROJECT_NAME}
    PLUGIN_NAME "Example Effect"
    FORMATS ${EXAMPLE_EFFECT_FORMATS}
    PLUGIN_CLASS ExampleEffect
    PLUGIN_CLASS_HEADER "ExampleEffect.h"
    RESOURCES
        resources/logo_transparent.png
        resources/mb-forever-raw.regular.ttf
    DATA_RESOURCES
        data/logo_transparent2.png

)
