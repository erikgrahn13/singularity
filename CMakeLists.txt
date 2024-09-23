cmake_minimum_required(VERSION 3.25)

# This needs to be included before project
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(FetchContent)
#include(VCPKG)
#include(SDL)
if(WIN32)
    include(ASIOSDK)
endif()

include(SKIA)
project(Singularity)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(core)
