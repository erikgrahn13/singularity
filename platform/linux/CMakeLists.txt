cmake_minimum_required(VERSION 3.22)

add_executable(main main3.cpp)

find_package(X11 REQUIRED)
target_link_libraries(main PRIVATE X11::X11)