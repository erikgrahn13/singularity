if(WIN32)
  set(GLFW_URL "https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.bin.WIN64.zip")
  set(GLFW_LIB "lib-vc2019/glfw3.lib")
elseif(APPLE)
  message("Add macos URL to glfw here")
elseif(UNIX)
    set(SFML_URL "https://www.sfml-dev.org/files/SFML-2.5.1-linux-gcc-64-bit.tar.gz")
    set(SFML_LIB)
else()
  message("Unsupported platform")
endif()



FetchContent_Declare(
  sfml
  URL ${SFML_URL}
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/sfml
)
FetchContent_MakeAvailable(sfml)

