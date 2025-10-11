# cmake_minimum_required(VERSION 3.14.0)

option(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES "Enable VST 3 Plug-in Examples" OFF)
option(SMTG_ENABLE_VST3_HOSTING_EXAMPLES "Enable VST 3 Hosting Examples" OFF)
option(SMTG_ENABLE_VSTGUI_SUPPORT "Enable VSTGUI Support" OFF)

# set(CMAKE_OSX_DEPLOYMENT_TARGET 10.13 CACHE STRING "")

# set(vst3sdk_SOURCE_DIR "C:/Users/erikg/Development/plugins/vst3sdk")

# if(NOT vst3sdk_SOURCE_DIR)
# message(FATAL_ERROR "Path to VST3 SDK is empty!")
# endif()

# project(SingularityEffect

# # This is your plug-in version number. Change it here only.
# # Version number symbols usable in C++ can be found in
# # source/version.h and ${PROJECT_BINARY_DIR}/projectversion.h.
# VERSION 1.0.0.0
# DESCRIPTION "SingularityEffect VST 3 Plug-in"
# )

# set(SMTG_VSTGUI_ROOT "${vst3sdk_SOURCE_DIR}")
set(SMTG_CUSTOM_BINARY_LOCATION "${CMAKE_BINARY_DIR}/${target}/vst3" CACHE PATH "Custom VST3 output location")
add_subdirectory(${vst3sdk_SOURCE_DIR} ${CMAKE_BINARY_DIR}/vst3sdk)

# list(APPEND CMAKE_MODULE_PATH "${vst3sdk_SOURCE_DIR}/cmake/modules")
# include(SMTG_VST3_SDK)
# add_subdirectory(${vst3sdk_SOURCE_DIR}/public.sdk/samples/vst-utilities ${CMAKE_BINARY_DIR}/vst3/moduleinfotool)
set(target_name ${target}_VST3)
message("erik7 ${target_name}")

smtg_enable_vst3_sdk()

smtg_add_vst3plugin_with_pkgname(${target_name} ${target}
    version.h
    cids.h
    processor.h
    processor.cpp
    controller.h
    controller.cpp
    entry.cpp
    vst3editor.h
    vst3editor.cpp
    ${CMAKE_SOURCE_DIR}/core/SingularityEditor.cpp
    ${CMAKE_SOURCE_DIR}/core/SingularityEditor.h
    ${CMAKE_SOURCE_DIR}/core/gui/singularity_Webview.h
    ${CMAKE_SOURCE_DIR}/core/gui/singularity_Webview.cpp
    ${CMAKE_SOURCE_DIR}/core/gui/singularity_ResourceManager.h
    ${CMAKE_SOURCE_DIR}/core/gui/singularity_ResourceManager.cpp
    ${CMAKE_SOURCE_DIR}/core/gui/platform/windows/singularityGUI_Windows.cpp
    ${CMAKE_SOURCE_DIR}/core/gui/platform/windows/singularityGUI_Windows.h
)

target_include_directories(${target_name} PUBLIC
    ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/vst3
    ${CMAKE_SOURCE_DIR}/core
)

# WebView2 setup for Windows
if(WIN32)
    # WebView2 SDK detection - multiple approaches
    set(WebView2_FOUND FALSE)

    # Method 1: Try to find via environment variables (NuGet packages)
    if(DEFINED ENV{USERPROFILE})
        set(NUGET_PACKAGES_DIR "$ENV{USERPROFILE}/.nuget/packages")
        file(GLOB WebView2_PACKAGE_DIRS "${NUGET_PACKAGES_DIR}/microsoft.web.webview2/*/")

        foreach(PACKAGE_DIR ${WebView2_PACKAGE_DIRS})
            if(EXISTS "${PACKAGE_DIR}/build/native/include/WebView2.h")
                set(WebView2_INCLUDE_DIR "${PACKAGE_DIR}/build/native/include")

                if(CMAKE_SIZEOF_VOID_P EQUAL 8)
                    set(WebView2_LIB_DIR "${PACKAGE_DIR}/build/native/x64")
                else()
                    set(WebView2_LIB_DIR "${PACKAGE_DIR}/build/native/x86")
                endif()

                set(WebView2_FOUND TRUE)
                message(STATUS "VST3: Found WebView2 via NuGet: ${WebView2_INCLUDE_DIR}")
                break()
            endif()
        endforeach()
    endif()

    # Method 2: Try Windows SDK locations
    if(NOT WebView2_FOUND)
        # Check common Windows SDK locations
        set(WIN_SDK_PATHS
            "C:/Program Files (x86)/Windows Kits/10/Include"
            "C:/Program Files/Windows Kits/10/Include"
        )

        foreach(SDK_PATH ${WIN_SDK_PATHS})
            file(GLOB SDK_VERSIONS "${SDK_PATH}/*/")

            foreach(VERSION_DIR ${SDK_VERSIONS})
                if(EXISTS "${VERSION_DIR}/winrt/WebView2.h")
                    set(WebView2_INCLUDE_DIR "${VERSION_DIR}/winrt")
                    set(WebView2_FOUND TRUE)
                    message(STATUS "VST3: Found WebView2 in Windows SDK: ${WebView2_INCLUDE_DIR}")
                    break()
                endif()
            endforeach()

            if(WebView2_FOUND)
                break()
            endif()
        endforeach()
    endif()

    # Configure libraries
    if(WebView2_FOUND AND DEFINED WebView2_LIB_DIR)
        set(WEBVIEW2_LIBS "${WebView2_LIB_DIR}/WebView2Loader.dll.lib" ole32 oleaut32 user32 gdi32 shlwapi)
        target_include_directories(${target_name} PRIVATE ${WebView2_INCLUDE_DIR})
    elseif(NOT WebView2_FOUND)
        message(WARNING "VST3: WebView2 SDK not found automatically. Trying fallback...")
        set(WEBVIEW2_LIBS webview2loader ole32 oleaut32 user32 gdi32 shlwapi)
    else()
        set(WEBVIEW2_LIBS webview2loader ole32 oleaut32 user32 gdi32)
    endif()
endif()

# - VSTGUI Wanted ----
# if(SMTG_ENABLE_VSTGUI_SUPPORT)
# target_sources(SingularityEffect
# PRIVATE
# resource/editor.uidesc
# )
# target_link_libraries(SingularityEffect
# PRIVATE
# vstgui_support
# )
# smtg_target_add_plugin_resources(SingularityEffect
# RESOURCES
# "resource/editor.uidesc"
# )
# endif(SMTG_ENABLE_VSTGUI_SUPPORT)
message("erik11 ${target}")

# -------------------
smtg_target_add_plugin_snapshots(${target_name}
    RESOURCES
    resource/E6C3724BE53B52308EA0160890E8716D_snapshot.png
    resource/E6C3724BE53B52308EA0160890E8716D_snapshot_2.0x.png
)

target_link_libraries(${target_name}
    PRIVATE
    sdk
    ${target}
    ${WEBVIEW2_LIBS}
)

smtg_target_configure_version_file(${target_name})

# if(SMTG_MAC)
# smtg_target_set_bundle(SingularityEffect
# BUNDLE_IDENTIFIER com.abysslounge.singularity
# COMPANY_NAME "FahlGrahn"
# )
# smtg_target_set_debug_executable(SingularityEffect
# "/Applications/VST3PluginTestHost.app"
# "--pluginfolder;$(BUILT_PRODUCTS_DIR)"
# )
# elseif(SMTG_WIN)
# target_sources(SingularityEffect PRIVATE
# resource/win32resource.rc
# )

# if(MSVC)
# set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT SingularityEffect)

# smtg_target_set_debug_executable(SingularityEffect
# "$(ProgramW6432)/Steinberg/VST3PluginTestHost/VST3PluginTestHost.exe"
# "--pluginfolder \"$(OutDir)/\""
# )
# endif()
# endif(SMTG_MAC)
