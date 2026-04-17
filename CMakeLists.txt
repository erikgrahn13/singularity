cmake_minimum_required(VERSION 3.20)

project(Singularity LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(FetchContent)

include(SKIA)
include(VST3)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded" CACHE STRING "" FORCE)
endif()



    # FetchContent_Declare(
    #     asiosdk
    #     URL https://www.steinberg.net/asiosdk
    #     SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/asiosdk
    # )

    # FetchContent_MakeAvailable(asiosdk)

    # add_library(asio STATIC
    #     ${asiosdk_SOURCE_DIR}/common/asio.cpp
    #     ${asiosdk_SOURCE_DIR}/host/asiodrivers.cpp
    #     ${asiosdk_SOURCE_DIR}/host/pc/asiolist.cpp
    # )
    # target_include_directories(asio PUBLIC 
    #     ${asiosdk_SOURCE_DIR}/common
    #     ${asiosdk_SOURCE_DIR}/host
    #     ${asiosdk_SOURCE_DIR}/host/pc    
    # )

FetchContent_Declare(
    dmon
    GIT_REPOSITORY https://github.com/septag/dmon.git
    GIT_TAG master
    GIT_SHALLOW TRUE
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/dmon
)

FetchContent_Populate(dmon)

FetchContent_Declare(
    choc
    GIT_REPOSITORY https://github.com/Tracktion/choc.git
    GIT_TAG main
    GIT_SHALLOW TRUE
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/choc
)
FetchContent_MakeAvailable(choc)




FetchContent_Declare(
    quickjs
    URL https://github.com/quickjs-ng/quickjs/archive/refs/tags/v0.12.1.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/quickjs
)

FetchContent_MakeAvailable(quickjs)

if(WIN32)
    add_executable(hello standalone/main.cpp)
    target_compile_definitions(hello PRIVATE NOMINMAX)
    target_sources(hello PRIVATE standalone/ASIO.cpp)

    target_link_libraries(hello PRIVATE ${skia_SOURCE_DIR}/win-gpu/lib/Release/x64/skia.lib asio)
    # set_property(TARGET hello PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
elseif(APPLE)
    add_executable(hello MACOSX_BUNDLE standalone/main.cpp)
    target_sources(hello PRIVATE platform/macos/CocoaWindow.mm standalone/coreAudio.cpp)
    target_link_libraries(hello PRIVATE
        ${skia_SOURCE_DIR}/mac-gpu/lib/Release/libskia.a
        "-framework AppKit"
    )
else()
    add_executable(hello standalone/main.cpp)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(Fontconfig REQUIRED IMPORTED_TARGET fontconfig)
    target_link_libraries(hello PRIVATE ${skia_SOURCE_DIR}/linux-gpu/lib/Release/x64/libskia.a PkgConfig::Fontconfig X11)

    # Optional Wayland support — generates xdg-shell protocol glue at configure time
    pkg_check_modules(WAYLAND_CLIENT IMPORTED_TARGET wayland-client)
    pkg_check_modules(WAYLAND_PROTOCOLS wayland-protocols)
    if(WAYLAND_CLIENT_FOUND AND WAYLAND_PROTOCOLS_FOUND)
        pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)
        find_program(WAYLAND_SCANNER wayland-scanner REQUIRED)

        set(XDG_SHELL_XML    "${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml")
        set(XDG_SHELL_HEADER "${CMAKE_BINARY_DIR}/xdg-shell-client-protocol.h")
        set(XDG_SHELL_CODE   "${CMAKE_BINARY_DIR}/xdg-shell-client-protocol.c")

        add_custom_command(
            OUTPUT  ${XDG_SHELL_HEADER}
            COMMAND ${WAYLAND_SCANNER} client-header ${XDG_SHELL_XML} ${XDG_SHELL_HEADER}
            DEPENDS ${XDG_SHELL_XML}
        )
        add_custom_command(
            OUTPUT  ${XDG_SHELL_CODE}
            COMMAND ${WAYLAND_SCANNER} private-code  ${XDG_SHELL_XML} ${XDG_SHELL_CODE}
            DEPENDS ${XDG_SHELL_XML}
        )

        add_custom_target(xdg_shell_protocol DEPENDS ${XDG_SHELL_HEADER} ${XDG_SHELL_CODE})
        add_dependencies(hello xdg_shell_protocol)

        target_sources(hello PRIVATE ${XDG_SHELL_CODE})
        target_include_directories(hello PRIVATE ${CMAKE_BINARY_DIR})
        target_link_libraries(hello PRIVATE PkgConfig::WAYLAND_CLIENT)
        target_compile_definitions(hello PRIVATE HAS_WAYLAND=1)
    endif()
endif()

target_link_libraries(hello PRIVATE 
    # SDL3::SDL3
    qjs-libc
    choc::choc
)

target_include_directories(hello PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${skia_SOURCE_DIR}/include
    ${quickjs_SOURCE_DIR}
    ${dmon_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/platform
)

target_compile_definitions(hello PRIVATE
    JS_SCRIPTS_DIR="${CMAKE_SOURCE_DIR}"
)

target_compile_definitions(hello PRIVATE SINGULARITY_STANDALONE=1)

target_sources(hello PRIVATE 
    SingularityGraphics2.cpp
    # dmonFileWatcher.cpp
    chocFileWatcher.cpp
    SkiaRenderer.cpp
    QuickJSEngine.cpp
    standalone/ISingularityAudio.cpp
    # chocQuickJSEngine.cpp
)

target_compile_features(hello PRIVATE cxx_std_23)

if(CMAKE_BUILD_TYPE STREQUAL Release)
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/hello_release.h
        COMMAND $<TARGET_FILE:qjsc>
            -o ${CMAKE_BINARY_DIR}/hello_release.h
            ${CMAKE_SOURCE_DIR}/hello.js
        DEPENDS qjsc hello.js
    )

    add_custom_target(generate_hello_release DEPENDS ${CMAKE_BINARY_DIR}/hello_release.h)
    add_dependencies(hello generate_hello_release)
    target_include_directories(hello PRIVATE ${CMAKE_BINARY_DIR})
endif()

target_link_libraries(hello PRIVATE 
    "-framework CoreAudio"
    "-framework CoreFoundation"
    "-framework AudioToolbox"
)

add_subdirectory(vst3)

# TODO: this is an example of how i can compile javascript files and modules with qjsc
# and then include the file in the in the cpp when building in release mode to embedd javascript
# qjsc -D widgets/button.js -D widgets/slider.js -o hello_release.h hello.js
# #include "hello_release.h"
#    js_std_eval_binary(ctx, qjsc_hello,qjsc_hello_size, 0);
# the qjsc command should probably be a add_custom_command in cmake