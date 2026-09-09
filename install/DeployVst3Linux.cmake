file(GET_RUNTIME_DEPENDENCIES
    MODULES "${VST3_MODULE}" "${QT_XCB_PLUGIN}"
    RESOLVED_DEPENDENCIES_VAR dependencies
    POST_INCLUDE_REGEXES ".*/libQt6[^/]*\\.so(\\..*)?$"
    POST_EXCLUDE_REGEXES ".*"
)

file(MAKE_DIRECTORY "${QT_DIR}")

file(COPY ${dependencies} "${QT_XCB_PLUGIN}"
    DESTINATION "${QT_DIR}"
    FOLLOW_SYMLINK_CHAIN
)

foreach(source IN LISTS dependencies QT_XCB_PLUGIN)
    get_filename_component(realSource "${source}" REALPATH)
    get_filename_component(filename "${realSource}" NAME)

    execute_process(
        COMMAND "${PATCHELF}" --set-rpath "$ORIGIN"
                "${QT_DIR}/${filename}"
        COMMAND_ERROR_IS_FATAL ANY
    )
endforeach()