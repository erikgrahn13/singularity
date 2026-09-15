file(REMOVE_RECURSE "${QT_DIR}")

include("${QML_IMPORTS_FILE}")

math(EXPR lastImport
    "${qml_import_scanner_imports_count} - 1")

set(qmlPlugins)

foreach(index RANGE ${lastImport})
    cmake_parse_arguments(qml "" "PATH;PLUGIN;RELATIVEPATH" ""
        ${qml_import_scanner_import_${index}})

    if(qml_PATH AND qml_PLUGIN)
        set(plugin "${qml_PATH}/lib${qml_PLUGIN}.so")
        set(destination "${QT_DIR}/qml/${qml_RELATIVEPATH}")

        file(COPY
            "${qml_PATH}/qmldir"
            "${plugin}"
            DESTINATION "${destination}"
            FOLLOW_SYMLINK_CHAIN
        )

        file(RELATIVE_PATH pathToQt "${destination}" "${QT_DIR}")

        execute_process(
            COMMAND "${PATCHELF}" --set-rpath
                "$ORIGIN/${pathToQt}"
                "${destination}/lib${qml_PLUGIN}.so"
            COMMAND_ERROR_IS_FATAL ANY
        )

        list(APPEND qmlPlugins "${plugin}")
    endif()
endforeach()

file(GET_RUNTIME_DEPENDENCIES
    MODULES "${VST3_MODULE}" "${QT_XCB_PLUGIN}" ${qmlPlugins}
    DIRECTORIES "${QT_LIBRARY_DIR}"
    RESOLVED_DEPENDENCIES_VAR dependencies
    POST_INCLUDE_REGEXES ".*/lib(Qt6|icu)[^/]*\\.so(\\..*)?$"
    POST_EXCLUDE_REGEXES ".*"
)

file(COPY ${dependencies} "${QT_XCB_PLUGIN}"
    DESTINATION "${QT_DIR}"
    FOLLOW_SYMLINK_CHAIN
)

foreach(source IN LISTS dependencies QT_XCB_PLUGIN)
    get_filename_component(source "${source}" REALPATH)
    get_filename_component(filename "${source}" NAME)

    execute_process(
        COMMAND "${PATCHELF}" --set-rpath "$ORIGIN"
            "${QT_DIR}/${filename}"
        COMMAND_ERROR_IS_FATAL ANY
    )
endforeach()