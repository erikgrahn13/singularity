add_executable(${target_name} main.swift)

set_target_properties(${target_name} PROPERTIES
    Swift_LANGUAGE_VERSION 6
    MACOSX_BUNDLE TRUE
)

# Add the required Swift compiler flag for @main
target_compile_options(${target_name} PRIVATE
    "$<$<COMPILE_LANGUAGE:Swift>:-parse-as-library>"
    "$<$<COMPILE_LANGUAGE:Swift>:-cxx-interoperability-mode=default>"
)
target_include_directories(${target_name} PUBLIC ${CMAKE_SOURCE_DIR}/core)