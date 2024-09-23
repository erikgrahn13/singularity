enable_language(Swift CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#find_package(unofficial-skia CONFIG REQUIRED)

add_executable(mainMacos MACOSX_BUNDLE mainMacosApp.swift)

add_library(cxx-support 
  coreAudio.cpp
  EditorMac.cpp
  EditorFactory.cpp
  ${SINGULARITY_CORE_PATH}/Editor.cpp
)
target_compile_options(cxx-support PRIVATE
  -fno-exceptions
  -fignore-exceptions)
target_include_directories(cxx-support PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}
  ${SINGULARITY_CORE_PATH}
  ${CMAKE_SOURCE_DIR}/external/skia_headers
  )
  target_compile_features(cxx-support PUBLIC cxx_std_20)

  target_link_libraries(cxx-support PUBLIC
  #unofficial::skia::skia
  ${skia_SOURCE_DIR}/${SKIA_LIB}
  "-framework CoreAudio"
  "-framework AudioUnit"  # AudioUnit framework is required for Core Audio I/O operations
)

target_compile_features(mainMacos PRIVATE cxx_std_20)
target_compile_options(mainMacos PRIVATE
  "SHELL:-cxx-interoperability-mode=default")
target_link_libraries(mainMacos PRIVATE
  cxx-support
  #unofficial::skia::skia
  ${skia_SOURCE_DIR}/${SKIA_LIB}
  "-framework SwiftUI"   # SwiftUI framework for macOS
  "-framework CoreAudio"  # CoreAudio framework
  "-framework AudioUnit"  # AudioUnit framework (if needed)
)

#add_dependencies(cxx-support skia_headers)
