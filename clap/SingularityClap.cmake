include_guard(GLOBAL)

FetchContent_Declare(
    clap
    GIT_REPOSITORY https://github.com/free-audio/clap.git
    GIT_TAG 1.2.10
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(clap)

FetchContent_Declare(
    clap-helpers
    GIT_REPOSITORY https://github.com/free-audio/clap-helpers
    GIT_TAG main
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(clap-helpers)

function(singularity_create_clap_plugin target)
    message("Erik CLAP")

    add_library(${target}_CLAP MODULE
        ${SINGULARITY_ROOT_DIR}/clap/entry.cpp
    )

    target_link_libraries(${target}_CLAP PRIVATE clap-helpers ${target})
    set_target_properties(${target}_CLAP PROPERTIES OUTPUT_NAME "${target}")

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # target_link_libraries(${target}_CLAP PRIVATE -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/linux-clap-plugins.version)
        # target_link_libraries(${target}_CLAP ${clap-helpers} ${target})
        target_link_libraries(${target}_CLAP PRIVATE -Wl,-z,defs)
        set_target_properties(${target}_CLAP PROPERTIES SUFFIX ".clap" PREFIX "")
    endif()


endfunction()
