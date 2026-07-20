cmake_minimum_required(VERSION 3.20)

project(ExampleInstrument
        VERSION 1.0.0.0
        DESCRIPTION "This is my example instrument plugin"
)

if(SINGULARITY_CAPI_SDK_DIR)
    set(EXAMPLE_INSTRUMENT_FORMATS CAPI)
else()
    set(EXAMPLE_INSTRUMENT_FORMATS APP VST3)
endif()

singularity_create_plugin(
    ${PROJECT_NAME}
    FORMATS ${EXAMPLE_INSTRUMENT_FORMATS}
    PLUGIN_CLASS ExampleInstrument
    PLUGIN_CLASS_HEADER "ExampleInstrument.h"
)
