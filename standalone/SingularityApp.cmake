include_guard(GLOBAL)

if(WIN32 AND NOT TARGET asio)
    FetchContent_Declare(
        asiosdk
        URL https://www.steinberg.net/asiosdk
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )

    FetchContent_MakeAvailable(asiosdk)

    add_library(asio STATIC
        ${asiosdk_SOURCE_DIR}/common/asio.cpp
        ${asiosdk_SOURCE_DIR}/host/asiodrivers.cpp
        ${asiosdk_SOURCE_DIR}/host/pc/asiolist.cpp
    )
    target_include_directories(asio PUBLIC
        ${asiosdk_SOURCE_DIR}/common
        ${asiosdk_SOURCE_DIR}/host
        ${asiosdk_SOURCE_DIR}/host/pc
    )
    # asiolist.cpp uses narrow-char APIs (CharLowerBuff etc.), while a combined
    # APP/VST3 build can globally define UNICODE through the VST3 SDK.
    target_compile_options(asio PRIVATE /UUNICODE /U_UNICODE)
    target_compile_definitions(asio PRIVATE _MBCS)
endif()

function(singularity_create_app_plugin target)
    set(oneValueArgs
        PLUGIN_CLASS
        PLUGIN_CLASS_HEADER
        BASE_TARGET
        SOURCE_DIR
        BINARY_DIR
        GENERATED_RESOURCES
        GENERATED_DATA_RESOURCES)
    set(multiValueArgs RESOURCES DATA_RESOURCES)
    cmake_parse_arguments(APP "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_executable(${target}_APP
        ${SINGULARITY_ROOT_DIR}/standalone/main.cpp
        ${SINGULARITY_ROOT_DIR}/standalone/ISingularityAudio.cpp
    )

    if(WIN32)
        target_sources(${target}_APP PRIVATE
            ${SINGULARITY_ROOT_DIR}/standalone/ASIO.cpp)
        target_compile_definitions(${target}_APP PRIVATE NOMINMAX)
        target_link_libraries(${target}_APP PRIVATE asio)
    elseif(APPLE)
        target_sources(${target}_APP PRIVATE
            ${SINGULARITY_ROOT_DIR}/standalone/coreAudio.cpp)
        target_link_libraries(${target}_APP PRIVATE
            "-framework CoreAudio"
            "-framework AudioToolbox"
        )
        set_target_properties(${target}_APP PROPERTIES MACOSX_BUNDLE TRUE)
    elseif(UNIX)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)
        target_sources(${target}_APP PRIVATE
            ${SINGULARITY_ROOT_DIR}/standalone/PipeWire.cpp)
        target_include_directories(${target}_APP PRIVATE ${PIPEWIRE_INCLUDE_DIRS})
        target_link_libraries(${target}_APP PRIVATE ${PIPEWIRE_LIBRARIES})
    endif()

    target_include_directories(${target}_APP PRIVATE
        ${SINGULARITY_ROOT_DIR}
        ${APP_SOURCE_DIR}
    )
    target_compile_definitions(${target}_APP PRIVATE
        SINGULARITY_STANDALONE=1
        PLUGIN_CLASS=${APP_PLUGIN_CLASS}
        PLUGIN_CLASS_HEADER="${APP_PLUGIN_CLASS_HEADER}"
    )
    target_link_libraries(${target}_APP PRIVATE ${APP_BASE_TARGET})

    if(APP_RESOURCES)
        target_compile_definitions(${target}_APP PRIVATE
            SINGULARITY_HAS_EMBEDDED_RESOURCES=1)
        target_sources(${target}_APP PRIVATE "${APP_GENERATED_RESOURCES}")
        target_include_directories(${target}_APP PRIVATE ${APP_BINARY_DIR})
    endif()

    if(APP_DATA_RESOURCES)
        target_compile_definitions(${target}_APP PRIVATE
            SINGULARITY_HAS_EMBEDDED_DATA_RESOURCES=1)
        target_sources(${target}_APP PRIVATE "${APP_GENERATED_DATA_RESOURCES}")
        target_include_directories(${target}_APP PRIVATE ${APP_BINARY_DIR})
    endif()

    set_target_properties(${target}_APP PROPERTIES
        OUTPUT_NAME "${target}"
        RUNTIME_OUTPUT_DIRECTORY "${APP_BINARY_DIR}/out/APP/$<CONFIG>"
    )
endfunction()
