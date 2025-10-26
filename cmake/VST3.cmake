FetchContent_Declare(
    vst3sdk
    GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk
    GIT_TAG v3.8.0_build_66
    GIT_SHALLOW TRUE
    GIT_SUBMODULES "base" "cmake" "pluginterfaces" "public.sdk"
    GIT_SUBMODULES_RECURSE FALSE
    GIT_SUBMODULES_SHALLOW TRUE
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/vst3sdk
    SOURCE_SUBDIR IGNORE
)

FetchContent_MakeAvailable(vst3sdk)
