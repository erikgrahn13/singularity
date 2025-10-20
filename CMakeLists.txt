cmake_minimum_required(VERSION 3.25)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SINGULARITY_CORE_PATH ${CMAKE_CURRENT_LIST_DIR}/core)

# This needs to be included before project
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(FetchContent)

if(WIN32)
    include(ASIOSDK)
    include(WebView2)
endif()

include(Miniz)
include(VST3)
include(SINGULARITY)

project(Singularity LANGUAGES CXX Swift)

add_subdirectory(example)
