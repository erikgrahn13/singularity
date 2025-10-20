
set(target_name ${target}_App)
add_executable(${target_name})

if(APPLE)
    target_sources(${target_name} PUBLIC
        macos/main.swift

        # ../bridge/SwiftEditorBridge.hpp # Add the bridge header
    )

    set_target_properties(${target_name} PROPERTIES
        Swift_LANGUAGE_VERSION 6
        MACOSX_BUNDLE TRUE
    )

    # Add the required Swift compiler flag for @main
    target_compile_options(${target_name} PRIVATE
        "$<$<COMPILE_LANGUAGE:Swift>:-parse-as-library>"
        "$<$<COMPILE_LANGUAGE:Swift>:-cxx-interoperability-mode=default>"
    )
    target_include_directories(${target_name} PUBLIC ${CMAKE_SOURCE_DIR}/core)

# #####################################################################
# If we haven't set the swift library search paths, do that now
# if(NOT SWIFT_LIBRARY_SEARCH_PATHS)
# if(CMAKE_OSX_SYSROOT)
# set(SDK_FLAGS "-sdk" "${CMAKE_OSX_SYSROOT}")
# endif()

# # Note: This does not handle cross-compiling correctly.
# # To handle it correctly, we would need to pass the target triple and
# # flags to this compiler invocation.
# execute_process(
# COMMAND ${CMAKE_Swift_COMPILER} ${SDK_FLAGS} -print-target-info
# OUTPUT_VARIABLE SWIFT_TARGET_INFO
# )

# # extract search paths from swift driver response
# string(JSON SWIFT_TARGET_PATHS GET ${SWIFT_TARGET_INFO} "paths")

# string(JSON SWIFT_TARGET_LIBRARY_PATHS GET ${SWIFT_TARGET_PATHS} "runtimeLibraryPaths")
# string(JSON SWIFT_TARGET_LIBRARY_PATHS_LENGTH LENGTH ${SWIFT_TARGET_LIBRARY_PATHS})
# math(EXPR SWIFT_TARGET_LIBRARY_PATHS_LENGTH "${SWIFT_TARGET_LIBRARY_PATHS_LENGTH} - 1 ")

# string(JSON SWIFT_TARGET_LIBRARY_IMPORT_PATHS GET ${SWIFT_TARGET_PATHS} "runtimeLibraryImportPaths")
# string(JSON SWIFT_TARGET_LIBRARY_IMPORT_PATHS_LENGTH LENGTH ${SWIFT_TARGET_LIBRARY_IMPORT_PATHS})
# math(EXPR SWIFT_TARGET_LIBRARY_IMPORT_PATHS_LENGTH "${SWIFT_TARGET_LIBRARY_IMPORT_PATHS_LENGTH} - 1 ")

# string(JSON SWIFT_SDK_IMPORT_PATH ERROR_VARIABLE errno GET ${SWIFT_TARGET_PATHS} "sdkPath")

# foreach(JSON_ARG_IDX RANGE ${SWIFT_TARGET_LIBRARY_PATHS_LENGTH})
# string(JSON SWIFT_LIB GET ${SWIFT_TARGET_LIBRARY_PATHS} ${JSON_ARG_IDX})
# list(APPEND SWIFT_SEARCH_PATHS ${SWIFT_LIB})
# endforeach()

# foreach(JSON_ARG_IDX RANGE ${SWIFT_TARGET_LIBRARY_IMPORT_PATHS_LENGTH})
# string(JSON SWIFT_LIB GET ${SWIFT_TARGET_LIBRARY_IMPORT_PATHS} ${JSON_ARG_IDX})
# list(APPEND SWIFT_SEARCH_PATHS ${SWIFT_LIB})
# endforeach()

# if(SWIFT_SDK_IMPORT_PATH)
# list(APPEND SWIFT_SEARCH_PATHS ${SWIFT_SDK_IMPORT_PATH})
# endif()

# # Save the swift library search paths
# set(SWIFT_LIBRARY_SEARCH_PATHS ${SWIFT_SEARCH_PATHS} CACHE FILEPATH "Swift driver search paths")
# endif()

# link_directories(${SWIFT_LIBRARY_SEARCH_PATHS})

# #####################################################################
elseif(WIN32)
    target_sources(${target_name} PUBLIC main.cpp)
endif()

target_link_libraries(${target_name} ${PLATFORM_LIBS} ${target})
