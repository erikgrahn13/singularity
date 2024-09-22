
add_executable(sdl_window main.cpp)

target_link_libraries(sdl_window PRIVATE  SDL2::SDL2-static SDL2::SDL2main)