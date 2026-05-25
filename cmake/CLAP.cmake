FetchContent_Declare(
    clap
    GIT_REPOSITORY https://github.com/free-audio/clap.git
    GIT_TAG 1.2.7
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    clap-helpers
    GIT_REPOSITORY https://github.com/free-audio/clap-helpers.git
    GIT_TAG main
    GIT_SHALLOW TRUE
)


FetchContent_MakeAvailable(clap)
FetchContent_MakeAvailable(clap-helpers)
