FetchContent_Declare(
    visage
    GIT_REPOSITORY https://github.com/VitalAudio/visage.git
    GIT_TAG main
    GIT_SHALLOW TRUE
    UPDATE_DISCONNECTED TRUE
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/visage
)

FetchContent_MakeAvailable(visage)
