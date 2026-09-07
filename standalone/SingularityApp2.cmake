include_guard(GLOBAL)

FetchContent_Declare(
    rtaudio
    GIT_REPOSITORY https://github.com/thestk/rtaudio.git
    GIT_TAG master
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(rtaudio)

function(singularity_create_app_plugin target)

    qt_add_executable(${target}_APP
        ${SINGULARITY_ROOT_DIR}/standalone/main2.cpp
        ${SINGULARITY_ROOT_DIR}/standalone/APPController.cpp
        # ${rtaudio_SOURCE_DIR}/RtAudio.cpp
        # ${SINGULARITY_ROOT_DIR}/standalone/ISingularityAudio.cpp
    )

    target_link_libraries(${target}_APP PRIVATE
        ${target}
        rtaudio
    )

    target_include_directories(${target}_APP PRIVATE
        ${SINGULARITY_ROOT_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${rtaudio_SOURCE_DIR}
    )

    # target_compile_definitions(${target}_APP PRIVATE
    #     PLUGIN_CLASS_HEADER="${PARAMS_PLUGIN_CLASS_HEADER}"
    #     PLUGIN_CLASS=${PARAMS_PLUGIN_CLASS}
    # )

    # if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    #     find_package(PkgConfig REQUIRED)
    #     pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)
    #     target_sources(${target}_APP PRIVATE ${SINGULARITY_ROOT_DIR}/standalone/PipeWire.cpp)
    #     target_include_directories(${target}_APP PRIVATE ${PIPEWIRE_INCLUDE_DIRS})
    #     target_link_libraries(${target}_APP PRIVATE ${PIPEWIRE_LIBRARIES})
    # elseif(APPLE)
    #     target_sources(${target}_APP PRIVATE ${SINGULARITY_ROOT_DIR}/standalone/coreAudio.cpp)
    #     target_link_libraries(${target}_APP PRIVATE
    #         "-framework CoreAudio"
    #         "-framework AudioToolbox"
    #     )
    #     set_target_properties(${target}_APP PROPERTIES MACOSX_BUNDLE TRUE)
    # elseif(WIN32)
    #     FetchContent_Declare(
    #         asiosdk
    #         URL https://www.steinberg.net/asiosdk
    #         DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    #     )

    #     FetchContent_MakeAvailable(asiosdk)

    #     add_library(asio STATIC
    #         ${asiosdk_SOURCE_DIR}/common/asio.cpp
    #         ${asiosdk_SOURCE_DIR}/host/asiodrivers.cpp
    #         ${asiosdk_SOURCE_DIR}/host/pc/asiolist.cpp
    #     )
    #     target_include_directories(asio PUBLIC
    #         ${asiosdk_SOURCE_DIR}/common
    #         ${asiosdk_SOURCE_DIR}/host
    #         ${asiosdk_SOURCE_DIR}/host/pc
    #     )

    #     target_sources(${target}_APP PRIVATE
    #         ${SINGULARITY_ROOT_DIR}/standalone/ASIO.cpp
    #         ${SINGULARITY_ROOT_DIR}/standalone/WASAPI.cpp
    #     )
        
    #     target_compile_definitions(${target}_APP PRIVATE NOMINMAX)
    #     target_link_libraries(${target}_APP PRIVATE asio avrt ole32 uuid)
    # endif()
endfunction()
