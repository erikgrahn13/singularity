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
)

target_include_directories(${target_name} PUBLIC
    ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/vst3
    ${CMAKE_SOURCE_DIR}/core
)

# -------------------
smtg_target_add_plugin_snapshots(${target_name}
    RESOURCES
    resource/E6C3724BE53B52308EA0160890E8716D_snapshot.png
    resource/E6C3724BE53B52308EA0160890E8716D_snapshot_2.0x.png
)

message("erik7 ${WEBVIEW2_LIBS}")

target_link_libraries(${target_name}
    PRIVATE
    sdk
    ${target}
    SingularityView
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
