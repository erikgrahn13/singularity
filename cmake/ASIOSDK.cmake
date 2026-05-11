FetchContent_Declare(
  asiosdk
  URL https://www.steinberg.net/asiosdk
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/asiosdk
)

FetchContent_MakeAvailable(asiosdk)

add_library(asio STATIC
  ${asiosdk_SOURCE_DIR}/common/asio.cpp
  ${asiosdk_SOURCE_DIR}/host/asiodrivers.cpp
  ${asiosdk_SOURCE_DIR}/host/pc/asiolist.cpp
)
target_include_directories(asio PUBLIC
  ${asiosdk_SOURCE_DIR}/common
  ${asiosdk_SOURCE_DIR}/host
  ${asiosdk_SOURCE_DIR}/host/pc
)