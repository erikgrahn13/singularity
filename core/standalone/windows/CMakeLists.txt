set(target_name ${target}_App)

add_executable(${target_name} WIN32 new_main.cpp)
target_link_libraries(${target_name} ${PLATFORM_LIBS} ${target} SingularityView)
target_include_directories(${target_name} PRIVATE ${CMAKE_SOURCE_DIR}/core/gui)
