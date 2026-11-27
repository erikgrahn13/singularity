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


if(WIN32)
  set(CLAP_VALIDATOR_URL "https://github.com/free-audio/clap-validator/releases/download/0.4.1/clap-validator-0.4.1-127-g152b982-windows.zip")
elseif(APPLE)
  set(CLAP_VALIDATOR_URL "https://github.com/free-audio/clap-validator/releases/download/0.4.1/clap-validator-0.4.1-127-g152b982-macos-15-aarch64.zip")
elseif(UNIX)
  set(CLAP_VALIDATOR_URL "https://github.com/free-audio/clap-validator/releases/download/0.4.1/clap-validator-0.4.1-127-g152b982-ubuntu-22.04.zip")
else()
  message(FATAL_ERROR "Not supported platform")
endif()

FetchContent_Declare(
    clap-validator
    URL ${CLAP_VALIDATOR_URL}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(clap-validator)

file(GLOB CLAP_VALIDATOR_ARCHIVE LIST_DIRECTORIES FALSE "${clap-validator_SOURCE_DIR}/*.tar.gz")

if(CLAP_VALIDATOR_ARCHIVE)
    file(ARCHIVE_EXTRACT INPUT "${CLAP_VALIDATOR_ARCHIVE}" DESTINATION "${clap-validator_SOURCE_DIR}")
endif()

function(singularity_create_clap_plugin target)

    add_library(${target}_CLAP MODULE
        ${SINGULARITY_ROOT_DIR}/clap/entry.cpp
    )

    target_link_libraries(${target}_CLAP PRIVATE clap-helpers ${target})
    set_target_properties(${target}_CLAP PROPERTIES OUTPUT_NAME "${target}")

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # target_link_libraries(${target}_CLAP PRIVATE -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/linux-clap-plugins.version)
        # target_link_libraries(${target}_CLAP ${clap-helpers} ${target})
        target_link_libraries(${target}_CLAP PRIVATE -Wl,-z,defs)
        set_target_properties(${target}_CLAP 
            PROPERTIES 
                SUFFIX ".clap" PREFIX ""
                CLAP_PLUGIN_PATH "$<TARGET_FILE:${target}_CLAP>"
        )
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        # target_link_options(clap-plugins PRIVATE -exported_symbols_list ${CMAKE_CURRENT_SOURCE_DIR}/macos-symbols.txt)
        set_target_properties(${target}_CLAP PROPERTIES
            BUNDLE True
            BUNDLE_EXTENSION clap
            MACOSX_BUNDLE_GUI_IDENTIFIER org.clap.example-plugins
            MACOSX_BUNDLE_BUNDLE_NAME clap-example-plugins
            MACOSX_BUNDLE_BUNDLE_VERSION "1"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "1"
            # MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/plugins.plist.in
            CLAP_PLUGIN_PATH "$<TARGET_BUNDLE_DIR:${target}_CLAP>"
        )

    elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set_target_properties(clap-plugins 
            PROPERTIES 
                SUFFIX ".clap" PREFIX ""
                CLAP_PLUGIN_PATH "$<TARGET_FILE:${target}_CLAP>"
        )
    endif()

    set_target_properties(${target}_CLAP
        PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/out/CLAP"
    )

    get_target_property(CLAP_PLUGIN ${target}_CLAP CLAP_PLUGIN_PATH)

    add_custom_command(
        TARGET ${target}_CLAP POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo [CLAP] Validator started...
        COMMAND
            ${clap-validator_SOURCE_DIR}/clap-validator validate ${CLAP_PLUGIN}
    )

endfunction()
