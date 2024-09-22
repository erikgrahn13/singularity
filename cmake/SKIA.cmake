if(WIN32)
  set(SKIA_URL "https://github.com/rust-skia/skia-binaries/releases/download/0.78.0/skia-binaries-dc17b3e9b36b91c342ef-x86_64-pc-windows-msvc-gl.tar.gz")
  set(SKIA_LIB "skia.lib")
elseif(APPLE)
  set(SKIA_URL "https://github.com/rust-skia/skia-binaries/releases/download/0.78.0/skia-binaries-dc17b3e9b36b91c342ef-aarch64-apple-darwin.tar.gz")
  set(SKIA_LIB "libskia.a")
elseif(UNIX)
  set(SKIA_URL "https://github.com/rust-skia/skia-binaries/releases/download/0.78.0/skia-binaries-dc17b3e9b36b91c342ef-x86_64-unknown-linux-gnu-textlayout.tar.gz")
  # set(SKIA_URL "https://github.com/rust-skia/skia-binaries/releases/download/0.78.0/skia-binaries-dc17b3e9b36b91c342ef-x86_64-unknown-linux-gnu.tar.gz")
  set(SKIA_LIB "libskia.a")
else()
  message("Unsupported platform")
endif()

FetchContent_Declare(
  skia
  URL ${SKIA_URL}
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/skia
)
FetchContent_MakeAvailable(skia)


FetchContent_Declare(
  skia_headers
  URL https://github.com/google/skia/archive/refs/heads/chrome/m129.zip
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/skia_headers
)
FetchContent_MakeAvailable(skia_headers)

# include(ExternalProject)
# ExternalProject_Add(
#     skia_headers
#     GIT_REPOSITORY https://github.com/google/skia.git
#     GIT_TAG chrome/m129  # Specify the desired commit or tag
#     GIT_SHALLOW TRUE       # Only fetch the latest commit
#     CONFIGURE_COMMAND ""
#     BUILD_COMMAND ""
#     INSTALL_COMMAND ""
#     UPDATE_COMMAND
#         git sparse-checkout init --cone &&
#         git sparse-checkout set include modules
#     LOG_DOWNLOAD ON
#     SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/skia-headers
# )

#add_dependencies(MyApp skia_headers)