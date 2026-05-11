FetchContent_GetProperties(dmon)
if(NOT dmon_POPULATED)
    FetchContent_Declare(
        dmon
        GIT_REPOSITORY https://github.com/septag/dmon.git
        GIT_TAG master
        GIT_SHALLOW TRUE
        # dmon is a single-header library — skip add_subdirectory entirely
        SOURCE_SUBDIR "__none__"
    )
    FetchContent_MakeAvailable(dmon)
endif()