# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find SFML package
#find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

# Add the executable
add_executable(sfml_window main.cpp)

# Link SFML libraries to the executable
target_link_libraries(sfml_window  ${sfml_SOURCE_DIR}/lib/libsfml-window.so ${sfml_SOURCE_DIR}/lib/libsfml-system.so)
target_include_directories(sfml_window PRIVATE ${sfml_SOURCE_DIR}/include)