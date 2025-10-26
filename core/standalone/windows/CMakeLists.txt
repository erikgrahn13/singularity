set(target_name ${target}_App)

add_executable(${target_name} WIN32 main.cpp)
target_link_libraries(${target_name} ${PLATFORM_LIBS} ${target} SingularityView SingularityAudio)
target_include_directories(${target_name} PRIVATE ${CMAKE_SOURCE_DIR}/core/gui)
target_compile_definitions(${target} PUBLIC PLUGIN_NAME="${target}")

add_subdirectory(audio)