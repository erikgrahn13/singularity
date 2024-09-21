if(WIN32)
  set(GLFW_URL "https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.bin.WIN64.zip")
  set(GLFW_LIB "lib-vc2019/glfw3.lib")
elseif(APPLE)
  message("Add macos URL to glfw here")
elseif(UNIX)
    message("Add linux URL to glfw here")
else()
  message("Unsupported platform")
endif()



FetchContent_Declare(
  glfw
  URL ${GLFW_URL}
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/glfw
)
FetchContent_MakeAvailable(glfw)

