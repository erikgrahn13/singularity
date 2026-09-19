if(POLICY CMP0207)
    cmake_policy(SET CMP0207 NEW)
endif()

if(DEPLOY_PLATFORM STREQUAL "Linux")
    file(REMOVE_RECURSE "${QT_DIR}")
    set(qmlRoot "${QT_DIR}/qml")
    set(qmlPluginPrefix "lib")
    set(qmlPluginSuffix ".so")
elseif(DEPLOY_PLATFORM STREQUAL "Darwin")
    file(REMOVE_RECURSE
        "${VST3_BUNDLE}/Contents/Frameworks"
        "${VST3_BUNDLE}/Contents/PlugIns"
        "${VST3_BUNDLE}/Contents/Resources/qml"
    )
    set(qmlRoot "${VST3_BUNDLE}/Contents/Resources/qml")
    set(qmlPluginDirectory "${VST3_BUNDLE}/Contents/PlugIns/quick")
    set(qmlPluginPrefix "lib")
    set(qmlPluginSuffix ".dylib")
elseif(DEPLOY_PLATFORM STREQUAL "Windows")
    file(REMOVE_RECURSE
        "${QT_DIR}/qml"
        "${QT_DIR}/platforms"
    )
    set(qmlRoot "${QT_DIR}/qml")
    set(qmlPluginPrefix "")
    set(qmlPluginSuffix ".dll")
endif()


include("${QML_IMPORTS_FILE}")

math(EXPR lastImport
    "${qml_import_scanner_imports_count} - 1")

set(qmlPlugins)
set(deployedQmlPlugins)

foreach(index RANGE ${lastImport})
    cmake_parse_arguments(qml "" "PATH;PLUGIN;RELATIVEPATH" ""
        ${qml_import_scanner_import_${index}})

    if(qml_PATH AND qml_PLUGIN)
        set(plugin "${qml_PATH}/${qmlPluginPrefix}${qml_PLUGIN}${qmlPluginSuffix}")
        set(moduleDirectory "${qmlRoot}/${qml_RELATIVEPATH}")

        set(pluginDirectory "${moduleDirectory}")

        if(DEFINED qmlPluginDirectory)
            set(pluginDirectory "${qmlPluginDirectory}")
        endif()

        file(COPY "${qml_PATH}/qmldir" DESTINATION "${moduleDirectory}")
        file(COPY "${plugin}" DESTINATION "${pluginDirectory}" FOLLOW_SYMLINK_CHAIN)
        list(APPEND qmlPlugins "${plugin}")

        if(DEPLOY_PLATFORM STREQUAL "Linux")
            file(RELATIVE_PATH pathToQt "${moduleDirectory}" "${QT_DIR}")

            execute_process(
                COMMAND "${PATCHELF}" --set-rpath
                    "$ORIGIN/${pathToQt}"
                    "${moduleDirectory}/${qmlPluginPrefix}${qml_PLUGIN}${qmlPluginSuffix}"
                COMMAND_ERROR_IS_FATAL ANY
            )
        elseif(DEPLOY_PLATFORM STREQUAL "Darwin")
            get_filename_component(pluginFilename "${plugin}" NAME)
            set(deployedPlugin "${pluginDirectory}/${pluginFilename}")
            file(RELATIVE_PATH pluginLink "${moduleDirectory}" "${deployedPlugin}")
            file(CREATE_LINK "${pluginLink}"
                "${moduleDirectory}/${pluginFilename}" SYMBOLIC)
            list(APPEND deployedQmlPlugins "${deployedPlugin}")
        endif()
    endif()
endforeach()

if(DEPLOY_PLATFORM STREQUAL "Linux")
    file(GET_RUNTIME_DEPENDENCIES
        MODULES "${VST3_MODULE}" "${QT_PLUGIN}" ${qmlPlugins}
        DIRECTORIES "${QT_LIBRARY_DIR}"
        RESOLVED_DEPENDENCIES_VAR dependencies
        POST_INCLUDE_REGEXES
            ".*/lib(Qt6|icu)[^/]*\\.so(\\..*)?$"
        POST_EXCLUDE_REGEXES ".*"
    )

    file(COPY ${dependencies} "${QT_PLUGIN}"
        DESTINATION "${QT_DIR}"
        FOLLOW_SYMLINK_CHAIN
    )

    foreach(source IN LISTS dependencies QT_PLUGIN)
        get_filename_component(source "${source}" REALPATH)
        get_filename_component(filename "${source}" NAME)

        execute_process(
            COMMAND "${PATCHELF}"
                --set-rpath "$ORIGIN"
                "${QT_DIR}/${filename}"
            COMMAND_ERROR_IS_FATAL ANY
        )
    endforeach()

elseif(DEPLOY_PLATFORM STREQUAL "Windows")
    file(GET_RUNTIME_DEPENDENCIES
        MODULES "${VST3_MODULE}" "${QT_PLUGIN}" ${qmlPlugins}
        DIRECTORIES "${QT_LIBRARY_DIR}"
        RESOLVED_DEPENDENCIES_VAR dependencies
        PRE_EXCLUDE_REGEXES
            "^api-ms-.*\\.dll$"
            "^ext-ms-.*\\.dll$"
        POST_EXCLUDE_REGEXES
            ".*/[Ww]indows/[Ss]ystem32/.*"
    )

    file(COPY ${dependencies}
        DESTINATION "${QT_DIR}"
    )

    # Qt's icuuc.dll is a small forwarding shim. CMake resolves the same name
    # from System32 first and the system-library filter then excludes it, so
    # copy Qt's shim explicitly when this Qt build provides one.
    if(EXISTS "${QT_LIBRARY_DIR}/icuuc.dll")
        file(COPY "${QT_LIBRARY_DIR}/icuuc.dll"
            DESTINATION "${QT_DIR}"
        )
    endif()

    file(COPY "${QT_PLUGIN}"
        DESTINATION "${QT_DIR}/platforms"
    )

elseif(DEPLOY_PLATFORM STREQUAL "Darwin")
    set(frameworkDirectory "${VST3_BUNDLE}/Contents/Frameworks")
    set(platformPluginDirectory "${VST3_BUNDLE}/Contents/PlugIns/platforms")

    file(COPY "${QT_PLUGIN}" DESTINATION "${platformPluginDirectory}"
        FOLLOW_SYMLINK_CHAIN)
    get_filename_component(platformPluginFilename "${QT_PLUGIN}" NAME)
    set(deployedPlatformPlugin
        "${platformPluginDirectory}/${platformPluginFilename}")

    file(GET_RUNTIME_DEPENDENCIES
        MODULES "${VST3_MODULE}" "${QT_PLUGIN}" ${qmlPlugins}
        DIRECTORIES "${QT_LIBRARY_DIR}"
        RESOLVED_DEPENDENCIES_VAR dependencies
        POST_EXCLUDE_REGEXES "^/System/Library/" "^/usr/lib/"
    )

    set(deployedLibraries)
    set(oldInstallNames)
    set(newInstallNames)

    foreach(source IN LISTS dependencies)
        if(source MATCHES "^(.+\\.framework)/Versions/([^/]+)/([^/]+)$")
            set(frameworkSource "${CMAKE_MATCH_1}")
            set(frameworkVersion "${CMAKE_MATCH_2}")
            set(frameworkBinaryName "${CMAKE_MATCH_3}")
            file(REAL_PATH "${frameworkSource}" frameworkSource)
            get_filename_component(frameworkName "${frameworkSource}" NAME)
            file(COPY "${frameworkSource}" DESTINATION "${frameworkDirectory}"
                PATTERN "Headers" EXCLUDE
                PATTERN "*.prl" EXCLUDE)
            set(deployedLibrary
                "${frameworkDirectory}/${frameworkName}/Versions/${frameworkVersion}/${frameworkBinaryName}")
            set(newInstallName
                "@rpath/${frameworkName}/Versions/${frameworkVersion}/${frameworkBinaryName}")
        else()
            get_filename_component(filename "${source}" NAME)
            file(COPY "${source}" DESTINATION "${frameworkDirectory}"
                FOLLOW_SYMLINK_CHAIN)
            set(deployedLibrary "${frameworkDirectory}/${filename}")
            set(newInstallName "@rpath/${filename}")
        endif()

        execute_process(
            COMMAND /usr/bin/otool -D "${source}"
            OUTPUT_VARIABLE installNameOutput
            COMMAND_ERROR_IS_FATAL ANY)
        string(REPLACE "\n" ";" installNameLines "${installNameOutput}")
        list(GET installNameLines 1 oldInstallName)
        string(STRIP "${oldInstallName}" oldInstallName)

        list(APPEND deployedLibraries "${deployedLibrary}")
        list(APPEND oldInstallNames "${oldInstallName}")
        list(APPEND newInstallNames "${newInstallName}")
    endforeach()

    set(allBinaries
        "${VST3_MODULE}"
        "${deployedPlatformPlugin}"
        ${deployedQmlPlugins}
        ${deployedLibraries})

    list(LENGTH oldInstallNames installNameCount)
    math(EXPR lastInstallName "${installNameCount} - 1")

    foreach(binary IN LISTS allBinaries)
        execute_process(
            COMMAND "${CODESIGN}" --remove-signature "${binary}"
            OUTPUT_QUIET ERROR_QUIET)

        set(changes)
        foreach(index RANGE ${lastInstallName})
            list(GET oldInstallNames ${index} oldInstallName)
            list(GET newInstallNames ${index} newInstallName)
            list(APPEND changes -change "${oldInstallName}" "${newInstallName}")
        endforeach()

        execute_process(
            COMMAND /usr/bin/otool -L "${binary}"
            OUTPUT_VARIABLE linkedLibraries
            COMMAND_ERROR_IS_FATAL ANY)
        string(REPLACE "\n" ";" linkedLibraryLines "${linkedLibraries}")

        foreach(linkedLibraryLine IN LISTS linkedLibraryLines)
            if(linkedLibraryLine MATCHES "^[ \t]+([^ \t]+)[ \t]+\\(")
                set(linkedInstallName "${CMAKE_MATCH_1}")
                get_filename_component(linkedFilename
                    "${linkedInstallName}" NAME)

                foreach(index RANGE ${lastInstallName})
                    list(GET deployedLibraries ${index} deployedLibrary)
                    get_filename_component(deployedFilename
                        "${deployedLibrary}" NAME)

                    if(linkedFilename STREQUAL deployedFilename)
                        list(GET newInstallNames ${index} newInstallName)
                        list(APPEND changes
                            -change "${linkedInstallName}" "${newInstallName}")
                        break()
                    endif()
                endforeach()
            endif()
        endforeach()

        execute_process(
            COMMAND "${INSTALL_NAME_TOOL}" ${changes} "${binary}"
            COMMAND_ERROR_IS_FATAL ANY)
    endforeach()

    foreach(index RANGE ${lastInstallName})
        list(GET deployedLibraries ${index} deployedLibrary)
        list(GET newInstallNames ${index} newInstallName)
        execute_process(
            COMMAND "${INSTALL_NAME_TOOL}" -id
                "${newInstallName}" "${deployedLibrary}"
            COMMAND_ERROR_IS_FATAL ANY)
    endforeach()

    execute_process(
        COMMAND "${INSTALL_NAME_TOOL}" -rpath "${QT_LIBRARY_DIR}"
            "@loader_path/../Frameworks" "${VST3_MODULE}"
        COMMAND_ERROR_IS_FATAL ANY)

    foreach(plugin IN LISTS deployedQmlPlugins deployedPlatformPlugin)
        execute_process(
            COMMAND "${INSTALL_NAME_TOOL}" -add_rpath
                "@loader_path/../../Frameworks" "${plugin}"
            COMMAND_ERROR_IS_FATAL ANY)
    endforeach()

    foreach(library IN LISTS deployedLibraries)
        if(NOT library MATCHES "\\.framework/")
            execute_process(
                COMMAND "${INSTALL_NAME_TOOL}" -add_rpath
                    "@loader_path" "${library}"
                COMMAND_ERROR_IS_FATAL ANY)
        endif()
    endforeach()

    foreach(binary IN LISTS allBinaries)
        execute_process(
            COMMAND /usr/bin/otool -l "${binary}"
            OUTPUT_VARIABLE loadCommands
            COMMAND_ERROR_IS_FATAL ANY)
        string(REGEX MATCHALL
            "path [^\n]+ \\(offset [0-9]+\\)"
            rpathEntries "${loadCommands}")

        foreach(rpathEntry IN LISTS rpathEntries)
            string(REGEX REPLACE
                "^path (.+) \\(offset [0-9]+\\)$" "\\1"
                rpath "${rpathEntry}")

            if(rpath MATCHES "^/opt/homebrew/" OR rpath MATCHES "^/Users/")
                execute_process(
                    COMMAND "${INSTALL_NAME_TOOL}" -delete_rpath
                        "${rpath}" "${binary}"
                    COMMAND_ERROR_IS_FATAL ANY)
            endif()
        endforeach()
    endforeach()

    execute_process(
        COMMAND "${CODESIGN}" --force --deep --sign - "${VST3_BUNDLE}"
        COMMAND_ERROR_IS_FATAL ANY)
endif()
