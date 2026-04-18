cmake_minimum_required(VERSION 3.20)

# Prebuilt libraries (Skia, QuickJS) are arm64-only on macOS
set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "" FORCE)

project(Singularity LANGUAGES C CXX)

# Must be set before ANY targets are created (including via FetchContent).
# Skia is a prebuilt /MT (release static) library, so everything must use /MT
# to avoid RuntimeLibrary and _ITERATOR_DEBUG_LEVEL mismatches on Windows.
if(WIN32)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded" CACHE STRING "" FORCE)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(FetchContent)

include(SKIA)
if(WIN32)
    include(ASIO)
endif()
include(DMON)
include(CHOC)
include(QUICKJS)
include(VST3)

set(SINGULARITY_SKIA_INCLUDE_DIR "${skia_SOURCE_DIR}/include" CACHE INTERNAL "")
set(SINGULARITY_QUICKJS_DIR "${quickjs_SOURCE_DIR}" CACHE INTERNAL "")
set(SINGULARITY_SKIA_LIB "${SKIA_LIB}" CACHE INTERNAL "")

include(SingularityPlugin)

if(WIN32)
    # add_executable(hello standalone/main.cpp)
    # target_compile_definitions(hello PRIVATE NOMINMAX)
    # target_sources(hello PRIVATE standalone/ASIO.cpp standalone/ISingularityAudio.cpp)


    # target_link_libraries(hello PRIVATE ${skia_SOURCE_DIR}/win-gpu/lib/Release/x64/skia.lib asio)
    # set_property(TARGET hello PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
elseif(APPLE)
    add_executable(hello MACOSX_BUNDLE standalone/main.cpp)
    target_sources(hello PRIVATE platform/macos/CocoaWindow.mm standalone/coreAudio.cpp standalone/ISingularityAudio.cpp)
    target_link_libraries(hello PRIVATE
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


target_include_directories(hello PRIVATE
    ${CMAKE_SOURCE_DIR}/platform
)

target_compile_definitions(hello PRIVATE
    SINGULARITY_STANDALONE=1
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
    Hello-shared
    "-framework CoreAudio"
    "-framework CoreFoundation"
    "-framework AudioToolbox"
)

# add_subdirectory(vst3)

# TODO: this is an example of how i can compile javascript files and modules with qjsc
# and then include the file in the in the cpp when building in release mode to embedd javascript
# qjsc -D widgets/button.js -D widgets/slider.js -o hello_release.h hello.js
# #include "hello_release.h"
#    js_std_eval_binary(ctx, qjsc_hello,qjsc_hello_size, 0);
# the qjsc command should probably be a add_custom_command in cmake