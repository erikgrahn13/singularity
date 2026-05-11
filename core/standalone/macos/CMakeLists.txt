add_executable(${target_name} main.swift)

set_target_properties(${target_name} PROPERTIES
    Swift_LANGUAGE_VERSION 6
    MACOSX_BUNDLE TRUE
    Swift_MODULE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${target_name}"
)

# Add the required Swift compiler flag for @main
target_compile_options(${target_name} PRIVATE
    "$<$<COMPILE_LANGUAGE:Swift>:-parse-as-library>"
    "$<$<COMPILE_LANGUAGE:Swift>:-cxx-interoperability-mode=default>"
)
target_include_directories(${target_name} PUBLIC ${CMAKE_SOURCE_DIR}/core)
target_link_libraries(${target_name} PRIVATE SingularityView ${target})