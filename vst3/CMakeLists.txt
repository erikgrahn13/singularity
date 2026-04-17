cmake_minimum_required(VERSION 3.15)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.13 CACHE STRING "")
# Prebuilt libraries (Skia, QuickJS) are arm64-only — restrict to arm64
set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "" FORCE)

project(HelloWorld
    VERSION 1.0.0.0
    DESCRIPTION "HelloWorld VST 3 Plug-in"
)
option(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES "Enable VST 3 Plug-in Examples" OFF)
option(SMTG_ENABLE_VST3_HOSTING_EXAMPLES "Enable VST 3 Hosting Examples" OFF)
option(SMTG_ENABLE_VSTGUI_SUPPORT "Enable VSTGUI Support" OFF)
option(JS_HOT_RELOAD "Watch and hot-reload JS scripts at runtime" ON)
# option(SMTG_USE_STATIC_CRT "use static CRuntime on Windows (option /MT)" ON)


add_subdirectory(${vst3sdk_SOURCE_DIR} ${CMAKE_BINARY_DIR}/vst3sdk)

smtg_enable_vst3_sdk()

smtg_add_vst3plugin(HelloWorld
    source/version.h
    source/helloworldcids.h
    source/helloworldprocessor.h
    source/helloworldprocessor.cpp
    source/helloworldcontroller.h
    source/helloworldcontroller.cpp
    source/helloworldentry.cpp
    source/SingularityView.h
    source/SingularityView.cpp
    ${CMAKE_SOURCE_DIR}/SingularityGraphics2.cpp
    ${CMAKE_SOURCE_DIR}/SkiaRenderer.cpp
    ${CMAKE_SOURCE_DIR}/QuickJSEngine.cpp
    ${CMAKE_SOURCE_DIR}/chocFileWatcher.cpp
)

if(APPLE)
    target_sources(HelloWorld PRIVATE ${CMAKE_SOURCE_DIR}/platform/macos/CocoaWindow.mm)
endif()

message("ERIK1 ${CMAKE_SOURCE_DIR}")

target_link_libraries(HelloWorld
    PRIVATE
        sdk
        qjs-libc
        choc::choc
)

if(WIN32)
    target_link_libraries(HelloWorld PRIVATE ${skia_SOURCE_DIR}/win-gpu/lib/Release/x64/skia.lib)
elseif(APPLE)
    target_link_libraries(HelloWorld PRIVATE
        ${skia_SOURCE_DIR}/mac-gpu/lib/Release/libskia.a
        "-framework AppKit"
        "-framework QuartzCore"
        "-framework CoreGraphics"
        "-framework CoreText"
        "-framework CoreFoundation"
        "-framework ImageIO"
        "-framework CoreServices"
    )
endif()

target_include_directories(HelloWorld PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${skia_SOURCE_DIR}/include
    ${quickjs_SOURCE_DIR}
    ${dmon_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/platform
)

target_compile_definitions(HelloWorld PRIVATE
    JS_SCRIPTS_DIR="${CMAKE_SOURCE_DIR}"
    NOMINMAX
)

if(JS_HOT_RELOAD)
    target_compile_definitions(HelloWorld PRIVATE JS_HOT_RELOAD)
endif()

target_compile_features(HelloWorld PRIVATE cxx_std_23)
# if(MSVC)
#     set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded" CACHE STRING "" FORCE)
# endif()

smtg_target_configure_version_file(HelloWorld)

if(SMTG_MAC)
    smtg_target_set_bundle(HelloWorld
        BUNDLE_IDENTIFIER net.steinberg.hello-world
        COMPANY_NAME "Steinberg Media Technologies GmbH"
    )
    smtg_target_set_debug_executable(HelloWorld
        "/Applications/VST3PluginTestHost.app"
        "--pluginfolder;$(BUILT_PRODUCTS_DIR)"
    )
elseif(SMTG_WIN)
    target_sources(HelloWorld PRIVATE 
        resource/win32resource.rc
    )
    if(MSVC)
        set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT HelloWorld)

        smtg_target_set_debug_executable(HelloWorld
            "$(ProgramW6432)/Steinberg/VST3PluginTestHost/VST3PluginTestHost.exe"
            "--pluginfolder \"$(OutDir)/\""
        )
    endif()
endif(SMTG_MAC)

