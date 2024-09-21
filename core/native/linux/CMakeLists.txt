set(CMAKE_CXX_STANDARD 11)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)

include_directories(${GTKMM_INCLUDE_DIRS})
link_directories(${GTKMM_LIBRARY_DIRS})
add_definitions(${GTKMM_CFLAGS_OTHER})

add_executable(my_gtkmm_app main.cpp)
target_link_libraries(my_gtkmm_app ${GTKMM_LIBRARIES} ${skia_SOURCE_DIR}/libskia.a)

add_dependencies(my_gtkmm_app skia_headers)
