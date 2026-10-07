include_guard(GLOBAL)

set(RTAUDIO_BUILD_SHARED_LIBS OFF CACHE BOOL "Build RtAudio as a static library")

if(WIN32)
    set(RTAUDIO_API_ASIO ON CACHE BOOL "Build RtAudio with ASIO support")
endif()

if(MSVC)
    set(RTAUDIO_STATIC_MSVCRT OFF CACHE BOOL "Use the dynamic MSVC runtime")
endif()

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
        WIN32 MACOSX_BUNDLE
        ${SINGULARITY_ROOT_DIR}/standalone/main.cpp
        ${SINGULARITY_ROOT_DIR}/standalone/APPController.cpp
    )

    target_link_libraries(${target}_APP PRIVATE
        ${target}
        rtaudio
    )

    target_compile_definitions(${target}_APP PRIVATE
        $<$<CONFIG:Debug>:QT_QML_DEBUG>
        $<$<CONFIG:Debug>:SINGULARITY_QML_SOURCE_FILE="${CMAKE_CURRENT_SOURCE_DIR}/Main.qml">
    )

    target_include_directories(${target}_APP PRIVATE
        ${SINGULARITY_ROOT_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${rtaudio_SOURCE_DIR}
    )

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(appComponent "${target}Standalone")
        set(appDeployOptions 
            NO_TRANSLATIONS

            EXCLUDE_PLUGINS
                qtvirtualkeyboardplugin

            EXCLUDE_PLUGIN_TYPES
                networkinformation
                printsupport
                qmltooling
                tls
                iconengines
                imageformats
                styles
        )

        if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
            list(APPEND appDeployOptions
                EXCLUDE_PLUGIN_TYPES
                    egldeviceintegrations
                    generic
                    platformthemes
                    wayland-decoration-client
                    wayland-graphics-integration-client
                    wayland-shell-integration

                POST_INCLUDE_REGEXES
                    ".*/libQt6.*"
                    ".*/libicu.*"
            )

            set(appLauncher "${CMAKE_CURRENT_BINARY_DIR}/${target}-launcher")

            file(GENERATE
                OUTPUT "${appLauncher}"
                CONTENT
        "#!/bin/sh

        appDir=\$(CDPATH= cd \"\$(dirname \"\$0\")\" && pwd)

        if [ -n \"\$LD_LIBRARY_PATH\" ]; then
            export LD_LIBRARY_PATH=\"\$appDir/../${CMAKE_INSTALL_LIBDIR}:\$LD_LIBRARY_PATH\"
        else
            export LD_LIBRARY_PATH=\"\$appDir/../${CMAKE_INSTALL_LIBDIR}\"
        fi

        exec \"\$appDir/${target}_APP\" \"\$@\"
        "
            )

            install(
                PROGRAMS "${appLauncher}"
                DESTINATION "${CMAKE_INSTALL_BINDIR}"
                RENAME "${target}"
                COMPONENT "${appComponent}"
            )
        endif()

        install(
            TARGETS ${target}_APP

            BUNDLE
                DESTINATION .
                COMPONENT "${appComponent}"

            RUNTIME
                DESTINATION "${CMAKE_INSTALL_BINDIR}"
                COMPONENT "${appComponent}"
        )

        qt_generate_deploy_qml_app_script(
            TARGET ${target}_APP
            OUTPUT_SCRIPT appDeployScript
            ${appDeployOptions}
        )

        install(
            SCRIPT "${appDeployScript}"
            COMPONENT "${appComponent}"
        )
    endif()
endfunction()
