FetchContent_Declare(
  asiosdk
  URL https://www.steinberg.net/asiosdk
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/asiosdk

)
FetchContent_MakeAvailable(asiosdk)