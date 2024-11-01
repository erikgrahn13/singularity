function(create_standalone target)
set(target_APP ${target}_APP)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
add_executable(${target_APP} WIN32
${SINGULARITY_CORE_PATH}/native/windows/mainWin.cpp
${SINGULARITY_CORE_PATH}/native/windows/EditorWin.cpp
${SINGULARITY_CORE_PATH}/Editor.cpp
${asiosdk_SOURCE_DIR}/host/pc/asiolist.cpp
${asiosdk_SOURCE_DIR}/host/asiodrivers.cpp
${asiosdk_SOURCE_DIR}/common/asio.cpp
)

target_include_directories(${target_APP} PRIVATE
    ${SINGULARITY_CORE_PATH}
    ${asiosdk_SOURCE_DIR}/common
    ${asiosdk_SOURCE_DIR}/host
    ${asiosdk_SOURCE_DIR}/host/pc
    ${skia_headers_SOURCE_DIR}
)

target_link_libraries(${target_APP} PRIVATE ${skia_SOURCE_DIR}/${SKIA_LIB})

# Configure runtime library options for Debug and Release builds
if(MSVC)
    # Force /MD for both Debug and Release builds to match Skia's release runtime
    target_compile_options(${target_APP} PRIVATE
        $<$<CONFIG:Debug>:/MD>   # Use /MD (Release runtime) in Debug build
        $<$<CONFIG:Release>:/MD> # Use /MD (Release runtime) in Release build
    )

    # Disable _ITERATOR_DEBUG_LEVEL for Debug builds, as Skia was built in Release mode
    add_definitions(-D_ITERATOR_DEBUG_LEVEL=0)
endif()

elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message("ADD LINUX HERE")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    enable_language(Swift CXX)

    add_executable(${target_APP} MACOSX_BUNDLE ${SINGULARITY_CORE_PATH}/native/macos/mainMacosApp.swift)

    add_library(cxx-support
    ${SINGULARITY_CORE_PATH}/native/macos/coreAudio.cpp
    ${SINGULARITY_CORE_PATH}/native/macos/EditorMac.cpp
    ${SINGULARITY_CORE_PATH}/native/macos/EditorFactory.cpp
    ${SINGULARITY_SOURCES}
    )
    target_compile_options(cxx-support PRIVATE
    -fno-exceptions
    -fignore-exceptions)
    target_include_directories(cxx-support PUBLIC
    ${SINGULARITY_CORE_PATH}/native/macos/
    ${SINGULARITY_CORE_PATH}
    ${CMAKE_SOURCE_DIR}/external/skia_headers
    )
    target_compile_features(cxx-support PUBLIC cxx_std_20)

    target_link_libraries(cxx-support PUBLIC
    ${skia_SOURCE_DIR}/${SKIA_LIB}
    ${target}
    "-framework CoreAudio"
    "-framework AudioUnit"  # AudioUnit framework is required for Core Audio I/O operations
    )

    target_compile_features(${target_APP} PRIVATE cxx_std_20)
    target_compile_options(${target_APP} PRIVATE
    "SHELL:-cxx-interoperability-mode=default")
    target_link_libraries(${target_APP} PRIVATE
    cxx-support
    ${skia_SOURCE_DIR}/${SKIA_LIB}
    "-framework SwiftUI"   # SwiftUI framework for macOS
    "-framework CoreAudio"  # CoreAudio framework
    "-framework AudioUnit"  # AudioUnit framework (if needed)
    )

endif()

endfunction()


function(singularity_create_plugin target)

    set(oneValueArgs PACKAGE_NAME)
    set(multiValueArgs SOURCES)
    # Parse the arguments
    cmake_parse_arguments(PARAMS "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Now handle the parsed arguments
    # If PACKAGE_NAME is not provided, use the target name as the default
    set(pkg_name "${PARAMS_PACKAGE_NAME}")
    if(NOT pkg_name)
        set(pkg_name ${target})
    endif()

    # If SOURCES is not provided, use the unparsed arguments (if any) as source files
    set(SOURCES "${PARAMS_SOURCES}")
    if(NOT SOURCES AND NOT PARAMS_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "No sources provided for target '${target}'. You must provide at least one source file.")
    endif()
    if(NOT SOURCES)
        set(SOURCES ${PARAMS_UNPARSED_ARGUMENTS})
    endif()

    set(SINGULARITY_SOURCES
        ${SINGULARITY_CORE_PATH}/Editor.h
        ${SINGULARITY_CORE_PATH}/Editor.cpp
    )

    add_library(${target} STATIC ${SOURCES})
    target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR}/external/skia_headers)
    create_standalone(${target})
    
endfunction()