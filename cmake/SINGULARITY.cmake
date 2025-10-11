# function(create_standalone_old target sources)
# set(target_APP ${target}_APP)

# if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
# add_executable(${target_APP} WIN32
# ${SINGULARITY_CORE_PATH}/native/windows/mainWin.cpp
# ${SINGULARITY_CORE_PATH}/native/windows/EditorWin.cpp
# ${SINGULARITY_CORE_PATH}/Editor.cpp
# ${sources}
# ${asiosdk_SOURCE_DIR}/host/pc/asiolist.cpp
# ${asiosdk_SOURCE_DIR}/host/asiodrivers.cpp
# ${asiosdk_SOURCE_DIR}/common/asio.cpp
# )

# target_include_directories(${target_APP} PRIVATE
# ${SINGULARITY_CORE_PATH}
# ${asiosdk_SOURCE_DIR}/common
# ${asiosdk_SOURCE_DIR}/host
# ${asiosdk_SOURCE_DIR}/host/pc
# ${skia_headers_SOURCE_DIR}
# )

# target_link_libraries(${target_APP} PRIVATE ${skia_SOURCE_DIR}/${SKIA_LIB})

# # Configure runtime library options for Debug and Release builds
# if(MSVC)
# # Force /MD for both Debug and Release builds to match Skia's release runtime
# target_compile_options(${target_APP} PRIVATE
# $<$<CONFIG:Debug>:/MD> # Use /MD (Release runtime) in Debug build
# $<$<CONFIG:Release>:/MD> # Use /MD (Release runtime) in Release build
# )

# # Disable _ITERATOR_DEBUG_LEVEL for Debug builds, as Skia was built in Release mode
# add_definitions(-D_ITERATOR_DEBUG_LEVEL=0)
# endif()

# elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
# message("ADD LINUX HERE")
# elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
# enable_language(Swift CXX)

# add_executable(${target_APP} MACOSX_BUNDLE ${SINGULARITY_CORE_PATH}/native/macos/mainMacosApp.swift)

# add_library(cxx-support
# ${SINGULARITY_CORE_PATH}/native/macos/coreAudio.cpp
# ${SINGULARITY_CORE_PATH}/native/macos/EditorMac.cpp
# ${SINGULARITY_CORE_PATH}/native/macos/EditorFactory.cpp
# ${SINGULARITY_SOURCES}
# )
# target_compile_options(cxx-support PRIVATE
# -fno-exceptions
# -fignore-exceptions)
# target_include_directories(cxx-support PUBLIC
# ${SINGULARITY_CORE_PATH}/native/macos/
# ${SINGULARITY_CORE_PATH}
# ${CMAKE_SOURCE_DIR}/external/skia_headers
# )
# target_compile_features(cxx-support PUBLIC cxx_std_20)

# target_link_libraries(cxx-support PUBLIC
# ${skia_SOURCE_DIR}/${SKIA_LIB}
# ${target}
# "-framework CoreAudio"
# "-framework AudioUnit" # AudioUnit framework is required for Core Audio I/O operations
# )

# target_compile_features(${target_APP} PRIVATE cxx_std_20)
# target_compile_options(${target_APP} PRIVATE
# "SHELL:-cxx-interoperability-mode=default")
# target_link_libraries(${target_APP} PRIVATE
# cxx-support
# ${skia_SOURCE_DIR}/${SKIA_LIB}
# "-framework SwiftUI" # SwiftUI framework for macOS
# "-framework CoreAudio" # CoreAudio framework
# "-framework AudioUnit" # AudioUnit framework (if needed)
# )
# endif()
# endfunction()

# function(create_plugins sources)
# message("erik3 ${sources}")
# set(CMAKE_OSX_DEPLOYMENT_TARGET "12.7" CACHE STRING "Minimum OS X deployment version" FORCE)
# add_subdirectory(${CMAKE_SOURCE_DIR}/core/plugins/VST3/SingularityEffect ${CMAKE_BINARY_DIR}/vst3)

# # target_sources(SingularityEffect PRIVATE ${CMAKE_SOURCE_DIR}/example/ExampleProcessor.h)
# target_include_directories(SingularityEffect PRIVATE ${CMAKE_SOURCE_DIR}/example)
# endfunction()

# function(create_standalone)
# add_subdirectory(${CMAKE_SOURCE_DIR}/core/gui ${CMAKE_BINARY_DIR}/${PROJECT_NAME})
# endfunction()
function(singularity_create_plugin target)
    set(oneValueArgs PACKAGE_NAME)
    set(multiValueArgs SOURCES FORMATS)

    # Parse the arguments
    cmake_parse_arguments(PARAMS "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Now handle the parsed arguments
    # If PACKAGE_NAME is not provided, use the target name as the default
    set(pkg_name "${PARAMS_PACKAGE_NAME}")

    if(NOT pkg_name)
        set(pkg_name ${target})
    endif()

    # If SOURCES is not provided, use the unparsed arguments (if any) as source files
    set(SOURCES "${PARAMS_SOURCES}")
    set(FORMATS "${PARAMS_FORMATS}")

    if(NOT SOURCES AND NOT PARAMS_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "No sources provided for target '${target}'. You must provide at least one source file.")
    endif()

    if(NOT SOURCES)
        set(SOURCES ${PARAMS_UNPARSED_ARGUMENTS})
    endif()

    set(SINGULARITY_SOURCES
        ${SINGULARITY_CORE_PATH}/SingularityEditor.h
        ${SINGULARITY_CORE_PATH}/SingularityEditor.cpp
        ${SINGULARITY_CORE_PATH}/SingularityProcessor.h
        ${SINGULARITY_CORE_PATH}/SingularityProcessor.cpp
        ${SINGULARITY_CORE_PATH}/gui/singularity_Webview.h
        ${SINGULARITY_CORE_PATH}/gui/singularity_Webview.cpp
        ${SINGULARITY_CORE_PATH}/gui/singularity_ResourceManager.h
        ${SINGULARITY_CORE_PATH}/gui/singularity_ResourceManager.cpp
    )

    message("erik1 ${SOURCES}")
    message("erik2 ${FORMATS}")

    # add_library(${target} STATIC ${SOURCES})
    # target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR}/external/skia_headers)
    # create_standalone(${target})
    # create_standalone(${target} "${SOURCES}")
    # create_plugins(${SOURCES})

    # #########################################################################################################
    # set(SOURCES
    # ../standalone/main.cpp
    # singularity_Webview.cpp
    # singularity_ResourceManager.cpp
    # )
    # set(target_name ${PROJECT_NAME}_App)
    if(APPLE)
        list(APPEND SOURCES ${CMAKE_SOURCE_DIR}/core/gui/platform/macos/webview_macos.mm)
        find_library(COCOA_FRAMEWORK Cocoa)
        find_library(WEBKIT_FRAMEWORK WebKit)
        set(PLATFORM_LIBS ${COCOA_FRAMEWORK} ${WEBKIT_FRAMEWORK})

        # Enable ARC for Objective-C++ files
        set_source_files_properties(src/platform/macos/webview_macos.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc")
    elseif(WIN32)
        list(APPEND SOURCES ${CMAKE_SOURCE_DIR}/core/gui/platform/windows/singularityGUI_Windows.cpp)

        set(initial_search_dir "$ENV{USERPROFILE}/AppData/Local/PackageManagement/NuGet/Packages")
        file(GLOB subdirs "${initial_search_dir}/*Microsoft.Web.WebView2*")

        if(subdirs)
            list(GET subdirs 0 search_dir)
            list(LENGTH subdirs num_webview2_packages)

            if(num_webview2_packages GREATER 1)
                message(WARNING "Multiple WebView2 packages found in the local NuGet folder. Proceeding with ${search_dir}.")
            endif()

            find_path(WebView2_root_dir build/native/include/WebView2.h HINTS ${search_dir})

            set(WebView2_include_dir "${WebView2_root_dir}/build/native/include")

            if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "arm64")
                set(WebView2_arch arm64)
            else()
                if(CMAKE_SIZEOF_VOID_P EQUAL 8)
                    set(WebView2_arch x64)
                elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
                    set(WebView2_arch x86)
                endif()
            endif()

            set(WebView2_library "${WebView2_root_dir}/build/native/${WebView2_arch}/WebView2LoaderStatic.lib")
        elseif(NOT WebView2_FIND_QUIETLY)
            message(WARNING
                "WebView2 wasn't found in the local NuGet folder."
                "\n"
                "To install NuGet and the WebView2 package containing the statically linked library, "
                "open a PowerShell and issue the following commands"
                "\n"
                "> Register-PackageSource -provider NuGet -name nugetRepository -location https://www.nuget.org/api/v2\n"
                "> Install-Package Microsoft.Web.WebView2 -Scope CurrentUser -RequiredVersion 1.0.1901.177 -Source nugetRepository\n"
                "\n")
        endif()

        find_package_handle_standard_args(WebView2 DEFAULT_MSG WebView2_include_dir WebView2_library)

        if(WebView2_FOUND)
            set(WebView2_INCLUDE_DIRS ${WebView2_include_dir})
            set(WebView2_LIBRARIES ${WebView2_library})

            mark_as_advanced(WebView2_library WebView2_include_dir WebView2_root_dir)

            if(NOT TARGET singularity_webview2)
                add_library(singularity_webview2 INTERFACE)
                add_library(singularity::singularity_webview2 ALIAS singularity_webview2)
                target_include_directories(singularity_webview2 INTERFACE ${WebView2_INCLUDE_DIRS})
                target_link_libraries(singularity_webview2 INTERFACE ${WebView2_LIBRARIES})
            endif()
        endif()

    # WebView2 SDK detection - multiple approaches
    # set(WebView2_FOUND FALSE)

    # # Method 1: Try to find via environment variables (NuGet packages)
    # if(DEFINED ENV{USERPROFILE})
    # set(NUGET_PACKAGES_DIR "$ENV{USERPROFILE}/.nuget/packages")
    # file(GLOB WebView2_PACKAGE_DIRS "${NUGET_PACKAGES_DIR}/microsoft.web.webview2/*/")

    # foreach(PACKAGE_DIR ${WebView2_PACKAGE_DIRS})
    # if(EXISTS "${PACKAGE_DIR}/build/native/include/WebView2.h")
    # set(WebView2_INCLUDE_DIR "${PACKAGE_DIR}/build/native/include")

    # if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    # set(WebView2_LIB_DIR "${PACKAGE_DIR}/build/native/x64")
    # else()
    # set(WebView2_LIB_DIR "${PACKAGE_DIR}/build/native/x86")
    # endif()

    # set(WebView2_FOUND TRUE)
    # message(STATUS "Found WebView2 via NuGet: ${WebView2_INCLUDE_DIR}")
    # break()
    # endif()
    # endforeach()
    # endif()

    # # Method 2: Try Windows SDK locations
    # if(NOT WebView2_FOUND)
    # # Check common Windows SDK locations
    # set(WIN_SDK_PATHS
    # "C:/Program Files (x86)/Windows Kits/10/Include"
    # "C:/Program Files/Windows Kits/10/Include"
    # )

    # foreach(SDK_PATH ${WIN_SDK_PATHS})
    # file(GLOB SDK_VERSIONS "${SDK_PATH}/*/")

    # foreach(VERSION_DIR ${SDK_VERSIONS})
    # if(EXISTS "${VERSION_DIR}/winrt/WebView2.h")
    # set(WebView2_INCLUDE_DIR "${VERSION_DIR}/winrt")
    # set(WebView2_FOUND TRUE)
    # message(STATUS "Found WebView2 in Windows SDK: ${WebView2_INCLUDE_DIR}")
    # break()
    # endif()
    # endforeach()

    # if(WebView2_FOUND)
    # break()
    # endif()
    # endforeach()
    # endif()

    # # # Method 3: vcpkg
    # # if(NOT WebView2_FOUND)
    # # find_package(unofficial-webview2 CONFIG QUIET)

    # # if(unofficial-webview2_FOUND)
    # # set(WebView2_FOUND TRUE)
    # # set(PLATFORM_LIBS unofficial::webview2::webview2 ole32 oleaut32 user32 gdi32 shlwapi)
    # # message(STATUS "Found WebView2 via vcpkg")
    # # endif()
    # # endif()

    # # Configure libraries
    # if(WebView2_FOUND AND DEFINED WebView2_LIB_DIR)
    # set(PLATFORM_LIBS "${WebView2_LIB_DIR}/WebView2Loader.dll.lib" ole32 oleaut32 user32 gdi32 shlwapi)
    # elseif(NOT WebView2_FOUND)
    # message(WARNING "WebView2 SDK not found automatically. Trying fallback...")
    # set(PLATFORM_LIBS webview2loader ole32 oleaut32 user32 gdi32 shlwapi)
    # else()
    # set(PLATFORM_LIBS webview2loader ole32 oleaut32 user32 gdi32)
    # endif()
    elseif(UNIX)
        list(APPEND SOURCES src/platform/linux/webview_linux.cpp)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(WEBKIT REQUIRED webkit2gtk-4.1)
        set(PLATFORM_LIBS ${WEBKIT_LIBRARIES})
        include_directories(${WEBKIT_INCLUDE_DIRS})
    endif()

    # #########################################################################################################
    list(APPEND SOURCES ${SINGULARITY_SOURCES})
    message("erik99 ${SOURCES}")

    add_library(${target} STATIC ${SOURCES})

    target_link_libraries(${target} PUBLIC singularity::singularity_webview2)

    foreach(type IN LISTS FORMATS)
        message("erik4 ${type}")

        if(type STREQUAL "Standalone")
            # create_standalone()
            add_subdirectory(${CMAKE_SOURCE_DIR}/core/standalone ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/app)
        elseif(type STREQUAL "VST3")
            # create_plugins(${SOURCES})
            set(CMAKE_OSX_DEPLOYMENT_TARGET "12.7" CACHE STRING "Minimum OS X deployment version" FORCE)
            add_subdirectory(${CMAKE_SOURCE_DIR}/core/plugins/VST3 ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/vst3)
        endif()
    endforeach()
endfunction()