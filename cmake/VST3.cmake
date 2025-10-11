FetchContent_Declare(
    vst3sdk
    GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk
    GIT_TAG v3.7.14_build_55
    GIT_SHALLOW TRUE
    GIT_SUBMODULES "base" "cmake" "pluginterfaces" "public.sdk"
    GIT_SUBMODULES_RECURSE FALSE
    GIT_SUBMODULES_SHALLOW TRUE
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/vst3sdk
    SOURCE_SUBDIR IGNORE
)

# Force C++17 for VST3 SDK to avoid C++20 u8string issues
# set(CMAKE_CXX_STANDARD 17)
# set(SMTG_CXX_STANDARD "17" CACHE STRING "" FORCE)
# add_compile_definitions(_SILENCE_CXX20_U8PATH_DEPRECATION_WARNING)
FetchContent_MakeAvailable(vst3sdk)

# set(CMAKE_CXX_STANDARD 23)
