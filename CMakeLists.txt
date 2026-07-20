cmake_minimum_required(VERSION 3.20)

# Default build type to Release if none was specified.
# Must come before project() so that generator-expression-based targets and
# VST3 SDK macros (_DEBUG / NDEBUG) are resolved correctly.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "No build type specified — defaulting to Release")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

# Prebuilt libraries (Skia, QuickJS) are arm64-only on macOS
set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "" FORCE)

project(Singularity LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(SingularityPlugin)

option(SINGULARITY_BUILD_EXAMPLES "Build the bundled example projects" OFF)
if(SINGULARITY_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# TODO: this is an example of how i can compile javascript files and modules with qjsc
# and then include the file in the in the cpp when building in release mode to embedd javascript
# qjsc -D widgets/button.js -D widgets/slider.js -o hello_release.h hello.js
# #include "hello_release.h"
#    js_std_eval_binary(ctx, qjsc_hello,qjsc_hello_size, 0);
# the qjsc command should probably be a add_custom_command in cmake
