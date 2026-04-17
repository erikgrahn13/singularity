FetchContent_Declare(
    dmon
    GIT_REPOSITORY https://github.com/septag/dmon.git
    GIT_TAG master
    GIT_SHALLOW TRUE
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/dmon
)

FetchContent_Populate(dmon)