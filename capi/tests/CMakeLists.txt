if(NOT SINGULARITY_CAPI_SDK_DIR)
    message(FATAL_ERROR
        "[singularity] CAPI tests require SINGULARITY_CAPI_SDK_DIR.")
endif()

add_executable(SingularityCapiTests
    CapiAdapterTests.cpp
)
target_compile_features(SingularityCapiTests PRIVATE cxx_std_23)
target_compile_definitions(SingularityCapiTests PRIVATE
    SINGULARITY_CAPI=1
    SINGULARITY_CAPI_MAX_BLOCK_SIZE=16
    SINGULARITY_CAPI_STACK_SIZE=4096
)
target_include_directories(SingularityCapiTests PRIVATE
    ${SINGULARITY_ROOT_DIR}
    ${SINGULARITY_ROOT_DIR}/capi
    ${SINGULARITY_ROOT_DIR}/examples/ExampleEffect
    ${SINGULARITY_ROOT_DIR}/examples/ExampleInstrument
    ${SINGULARITY_CAPI_SDK_DIR}/ar_osal/api
    ${SINGULARITY_CAPI_SDK_DIR}/fwk/api/modules
    ${SINGULARITY_CAPI_SDK_DIR}/fwk/spf/interfaces/module/capi
    ${SINGULARITY_CAPI_SDK_DIR}/fwk/api/ar_utils
    ${SINGULARITY_CAPI_SDK_DIR}/fwk/spf/interfaces/module/metadata/api
)

add_test(NAME SingularityCapiTests COMMAND SingularityCapiTests)
