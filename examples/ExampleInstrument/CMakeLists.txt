cmake_minimum_required(VERSION 3.20)

project(ExampleInstrument
        VERSION 1.0.0.0
        DESCRIPTION "This is my example instrument plugin"
)

singularity_create_plugin(
    ${PROJECT_NAME}
    FORMATS APP VST3
    PLUGIN_CLASS ExampleInstrument
    PLUGIN_CLASS_HEADER "ExampleInstrument.h"
)
