FetchContent_Declare(
    visage
    GIT_REPOSITORY https://github.com/VitalAudio/visage.git
    GIT_TAG main
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(visage)
