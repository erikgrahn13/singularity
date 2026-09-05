include_guard(GLOBAL)

function(singularity_create_app_plugin target)

    qt_add_executable(${target}_APP
        ${SINGULARITY_ROOT_DIR}/standalone/main2.cpp
        ${SINGULARITY_ROOT_DIR}/standalone/ISingularityAudio.cpp
    )

    target_link_libraries(${target}_APP PRIVATE
        ${target}
    )

    target_include_directories(${target}_APP PRIVATE
        ${SINGULARITY_ROOT_DIR}
        ${SINGULARITY_ROOT_DIR}/standalone
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

    target_compile_definitions(${target}_APP PRIVATE
        PLUGIN_CLASS_HEADER="${PARAMS_PLUGIN_CLASS_HEADER}"
        PLUGIN_CLASS=${PARAMS_PLUGIN_CLASS}
    )

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)
    target_sources(${target}_APP PRIVATE ${SINGULARITY_ROOT_DIR}/standalone/PipeWire.cpp)
    target_include_directories(${target}_APP PRIVATE ${PIPEWIRE_INCLUDE_DIRS})
    target_link_libraries(${target}_APP PRIVATE ${PIPEWIRE_LIBRARIES})
endfunction()