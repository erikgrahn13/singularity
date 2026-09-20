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
        ${SINGULARITY_ROOT_DIR}/standalone/main2.cpp
        ${SINGULARITY_ROOT_DIR}/standalone/APPController.cpp
        # ${rtaudio_SOURCE_DIR}/RtAudio.cpp
        # ${SINGULARITY_ROOT_DIR}/standalone/ISingularityAudio.cpp
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

        # if(APPLE)
        #     list(APPEND appDeployOptions
        #         # MACOS_BUNDLE_POST_BUILD
        #     )
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

            # FetchContent_Declare(
            #     patchelf_0191
            #     URL
            #         "https://github.com/NixOS/patchelf/releases/download/0.19.1/patchelf-0.19.1-x86_64.tar.gz"
            #     URL_HASH
            #         SHA256=a6818fef80128fb354423234ecacdcca3e993913d774e5d8346bc63f70fed4cf
            #     DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            # )

            # FetchContent_MakeAvailable(patchelf_0191)

            # set(QT_DEPLOY_PATCHELF_EXECUTABLE "${patchelf_0191_SOURCE_DIR}/bin/patchelf" CACHE FILEPATH "patchelf used by Qt deployment")

            # file(CHMOD "${QT_DEPLOY_PATCHELF_EXECUTABLE}"
            #     PERMISSIONS
            #         OWNER_READ OWNER_WRITE OWNER_EXECUTE
            #         GROUP_READ GROUP_EXECUTE
            #         WORLD_READ WORLD_EXECUTE
            # )

            # set(QT_DEPLOY_USE_PATCHELF ON CACHE BOOL "Use patchelf during Qt deployment")

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







        # if(CMAKE_SYSTEM_NAME STREQUAL "Linux")

        #     FetchContent_Declare(
        #         patchelf_0191
        #         URL
        #             "https://github.com/NixOS/patchelf/releases/download/0.19.1/patchelf-0.19.1-x86_64.tar.gz"
        #         URL_HASH
        #             SHA256=a6818fef80128fb354423234ecacdcca3e993913d774e5d8346bc63f70fed4cf
        #     )

        #     FetchContent_MakeAvailable(patchelf_0191)

        #     set(
        #         PATCHELF_EXECUTABLE
        #         "${patchelf_0191_SOURCE_DIR}/bin/patchelf"
        #     )

        #     file(CHMOD "${PATCHELF_EXECUTABLE}"
        #         PERMISSIONS
        #             OWNER_READ OWNER_WRITE OWNER_EXECUTE
        #             GROUP_READ GROUP_EXECUTE
        #             WORLD_READ WORLD_EXECUTE
        #     )


        #     FetchContent_Declare(
        #         linuxdeploy
        #         URL
        #             "https://github.com/linuxdeploy/linuxdeploy/releases/download/1-alpha-20251107-1/linuxdeploy-x86_64.AppImage"
        #         DOWNLOAD_NO_EXTRACT TRUE
        #         TLS_VERIFY TRUE
        #     )
        #     FetchContent_Declare(
        #         linuxdeploy_plugin_qt
        #         URL
        #             "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/1-alpha-20250213-1/linuxdeploy-plugin-qt-x86_64.AppImage"
        #         DOWNLOAD_NO_EXTRACT TRUE
        #         TLS_VERIFY TRUE
        #     )

        #     FetchContent_MakeAvailable(linuxdeploy linuxdeploy_plugin_qt)
        #     set(LINUXDEPLOY_EXECUTABLE "${linuxdeploy_SOURCE_DIR}/linuxdeploy-x86_64.AppImage")
        #     set(LINUXDEPLOY_PLUGIN_QT_EXECUTABLE "${linuxdeploy_plugin_qt_SOURCE_DIR}/linuxdeploy-plugin-qt-x86_64.AppImage")

        #     file(CHMOD
        #         "${LINUXDEPLOY_EXECUTABLE}"
        #         "${LINUXDEPLOY_PLUGIN_QT_EXECUTABLE}"
        #         PERMISSIONS
        #             OWNER_READ OWNER_WRITE OWNER_EXECUTE
        #             GROUP_READ GROUP_EXECUTE
        #             WORLD_READ WORLD_EXECUTE
        #     )

        #     set(appDir "${CMAKE_CURRENT_BINARY_DIR}/${target}.AppDir")
        #     set(appImage "${CMAKE_CURRENT_BINARY_DIR}/${target}-${PROJECT_VERSION}-x86_64.AppImage")
        #     set(desktopFile "${CMAKE_CURRENT_BINARY_DIR}/${target}.desktop")
        #     set(appIcon "${SINGULARITY_ROOT_DIR}/resources/logo_transparent_512.png")

        #     file(GENERATE
        #         OUTPUT "${desktopFile}"
        #         CONTENT
        #             "[Desktop Entry]\nType=Application\nName=${target}\nExec=$<TARGET_FILE_NAME:${target}_APP>\nIcon=${target}\nCategories=AudioVideo;Audio;\n"
        #     )

        #     add_custom_command(
        #         TARGET ${target}_APP
        #         POST_BUILD

        #         COMMAND "${CMAKE_COMMAND}" -E rm -rf
        #             "${appDir}"

        #         COMMAND "${CMAKE_COMMAND}" -E env
        #             --unset=DEBUG
        #             NO_STRIP=1
        #             "PATCHELF=${PATCHELF_EXECUTABLE}"
        #             "PATH=${linuxdeploy_plugin_qt_SOURCE_DIR}:$ENV{PATH}"
        #             "QMAKE=$<TARGET_FILE:Qt6::qmake>"
        #             "QML_SOURCES_PATHS=${CMAKE_CURRENT_SOURCE_DIR}"
        #             "LINUXDEPLOY_OUTPUT_APP_NAME=${target}"
        #             "LINUXDEPLOY_OUTPUT_VERSION=${PROJECT_VERSION}"
        #             "LDAI_OUTPUT=${appImage}"
        #             "${LINUXDEPLOY_EXECUTABLE}"
        #             --verbosity=2
        #             --appdir "${appDir}"
        #             --executable "$<TARGET_FILE:${target}_APP>"
        #             --desktop-file "${desktopFile}"
        #             --icon-file "${appIcon}"
        #             --icon-filename "${target}"
        #             --plugin qt
        #             --output appimage

        #         BYPRODUCTS
        #             "${appImage}"

        #         VERBATIM
        #         COMMENT "Creating ${target} AppImage"
        #     )
        # endif()
    endif()

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
