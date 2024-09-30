cmake_minimum_required(VERSION 3.25)

set(SINGULARITY_CORE_PATH ${CMAKE_CURRENT_LIST_DIR}/core)

# This needs to be included before project
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(FetchContent)
if(WIN32)
    include(ASIOSDK)
endif()
include(SKIA)
include(SINGULARITY)

project(Singularity)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(example)
