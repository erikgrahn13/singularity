set(target_name ${target}_App)

add_executable(${target_name} main.cpp)

target_include_directories(${target_name} PRIVATE 
    ${CMAKE_SOURCE_DIR}/core/gui
)
target_link_libraries(${target_name} PUBLIC 
    ${target}
    SingularityView
)
