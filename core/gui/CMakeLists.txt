# cmake_minimum_required(VERSION 3.16)
# project(NativeWebView LANGUAGES CXX)

# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)

message("erik3 ${target}")

set(SOURCES
    ../standalone/main.cpp
    singularity_Webview.cpp
    singularity_ResourceManager.cpp
)

set(target_name ${PROJECT_NAME}_App)

if(APPLE)
    list(APPEND SOURCES src/platform/macos/webview_macos.mm)
    find_library(COCOA_FRAMEWORK Cocoa)
    find_library(WEBKIT_FRAMEWORK WebKit)
    set(PLATFORM_LIBS ${COCOA_FRAMEWORK} ${WEBKIT_FRAMEWORK})

    # Enable ARC for Objective-C++ files
    set_source_files_properties(src/platform/macos/webview_macos.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc")
elseif(WIN32)
    list(APPEND SOURCES platform/windows/singularityGUI_Windows.cpp)

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
                message(STATUS "Found WebView2 via NuGet: ${WebView2_INCLUDE_DIR}")
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
                    message(STATUS "Found WebView2 in Windows SDK: ${WebView2_INCLUDE_DIR}")
                    break()
                endif()
            endforeach()

            if(WebView2_FOUND)
                break()
            endif()
        endforeach()
    endif()

    # # Method 3: vcpkg
    # if(NOT WebView2_FOUND)
    # find_package(unofficial-webview2 CONFIG QUIET)

    # if(unofficial-webview2_FOUND)
    # set(WebView2_FOUND TRUE)
    # set(PLATFORM_LIBS unofficial::webview2::webview2 ole32 oleaut32 user32 gdi32 shlwapi)
    # message(STATUS "Found WebView2 via vcpkg")
    # endif()
    # endif()

    # Configure libraries
    if(WebView2_FOUND AND DEFINED WebView2_LIB_DIR)
        set(PLATFORM_LIBS "${WebView2_LIB_DIR}/WebView2Loader.dll.lib" ole32 oleaut32 user32 gdi32 shlwapi)
    elseif(NOT WebView2_FOUND)
        message(WARNING "WebView2 SDK not found automatically. Trying fallback...")
        set(PLATFORM_LIBS webview2loader ole32 oleaut32 user32 gdi32 shlwapi)
    else()
        set(PLATFORM_LIBS webview2loader ole32 oleaut32 user32 gdi32)
    endif()
elseif(UNIX)
    list(APPEND SOURCES src/platform/linux/webview_linux.cpp)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(WEBKIT REQUIRED webkit2gtk-4.1)
    set(PLATFORM_LIBS ${WEBKIT_LIBRARIES})
    include_directories(${WEBKIT_INCLUDE_DIRS})
endif()

# add_executable(${target_name} ${SOURCES} ${SINGULARITY_SOURCES})
# target_link_libraries(${target_name} ${PLATFORM_LIBS} ${target})

# # Set include directories after target creation (Windows WebView2)
# if(WIN32 AND WebView2_FOUND AND DEFINED WebView2_INCLUDE_DIR)
# target_include_directories(${target_name} PRIVATE ${WebView2_INCLUDE_DIR})
# endif()

# # Include directories
# target_include_directories(${target_name} PRIVATE src)