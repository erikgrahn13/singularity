cmake_minimum_required(VERSION 3.25)

include(FetchContent)

FetchContent_Declare(
  vcpkg
  GIT_REPOSITORY https://github.com/microsoft/vcpkg.git
  GIT_TAG 2024.08.23
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/external/vcpkg
)
FetchContent_MakeAvailable(vcpkg)

set(CMAKE_TOOLCHAIN_FILE "${vcpkg_SOURCE_DIR}/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "")

project(Singularity LANGUAGES Swift)

# add_executable(windows WIN32
#     mainWin.cpp
#     ${asio_SOURCE_DIR}/host/pc/asiolist.cpp
#     ${asio_SOURCE_DIR}/host/asiodrivers.cpp
#     ${asio_SOURCE_DIR}/common/asio.cpp
# )

# target_include_directories(windows PRIVATE 
#     ${asio_SOURCE_DIR}/common
#     ${asio_SOURCE_DIR}/host
#     ${asio_SOURCE_DIR}/host/pc
# )


# SWIFTUI
# Enable macOS app bundle generation
set(CMAKE_MACOSX_BUNDLE TRUE)

# Specify the minimum macOS version required
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0") # macOS 11.0 is required for SwiftUI 2.0

# Find the Swift package (built-in support for Swift in CMake)
#find_package(Swift REQUIRED)

# Add the Swift source files
set(SWIFT_SOURCES
    macos2/mainMacos/mainMacos/mainMacosApp.swift
)

# Add executable for the macOS app bundle
add_executable(mainMacos MACOSX_BUNDLE ${SWIFT_SOURCES})

# Add the required frameworks for SwiftUI and macOS apps
target_link_libraries(mainMacos
    "-framework SwiftUI"
    #"-framework AppKit"
)

# Specify the bundle properties (optional: set your own icons, etc.)
# set_target_properties(mainMacos PROPERTIES
#     MACOSX_BUNDLE_GUI_IDENTIFIER "com.example.mainMacos"
#     MACOSX_BUNDLE_BUNDLE_NAME "mainMacos"
#     MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/Info.plist" # Add your Info.plist here
# )

# Copy the asset catalog and other resources into the bundle
set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/macos2/mainMacos/mainMacos/Assets.xcassets")
set(ENTITLEMENTS_FILE "${CMAKE_SOURCE_DIR}/macos2/mainMacos/mainMacos/mainMacos.entitlements")

# Ensure the assets and entitlements are included in the app bundle
target_sources(mainMacos PRIVATE
    ${ASSETS_DIR}
    ${ENTITLEMENTS_FILE}
)

# Configure the app's resource path to include assets
set_source_files_properties(${ASSETS_DIR} PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources"
)