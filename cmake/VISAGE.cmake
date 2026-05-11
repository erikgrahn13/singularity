FetchContent_Declare(
    visage
    GIT_REPOSITORY https://github.com/VitalAudio/visage.git
    GIT_TAG main
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(visage)

# On Linux, visage is linked into a VST3 .so — must be compiled with -fPIC.
if(UNIX AND NOT APPLE)
    set_target_properties(visage PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
