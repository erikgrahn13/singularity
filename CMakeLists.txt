cmake_minimum_required(VERSION 3.25)

include(FetchContent)

FetchContent_Declare(
  asio
  URL https://www.steinberg.net/asiosdk
)
FetchContent_MakeAvailable(asio)

add_executable(windows WIN32
    mainWin.cpp
    ${asio_SOURCE_DIR}/host/pc/asiolist.cpp
    ${asio_SOURCE_DIR}/host/asiodrivers.cpp
    ${asio_SOURCE_DIR}/common/asio.cpp
)

target_include_directories(windows PRIVATE 
    ${asio_SOURCE_DIR}/common
    ${asio_SOURCE_DIR}/host
    ${asio_SOURCE_DIR}/host/pc
)