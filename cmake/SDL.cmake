# Disable unused subsystems
set(SDL_AUDIO OFF CACHE BOOL "Disable SDL audio subsystem")
set(SDL_VIDEO_VULKAN OFF CACHE BOOL "Disable Vulkan support")
set(SDL_RENDER OFF CACHE BOOL "Disable SDL renderer subsystem")
set(SDL_HAPTIC OFF CACHE BOOL "Disable SDL haptic (force feedback) subsystem")
set(SDL_JOYSTICK OFF CACHE BOOL "Disable SDL joystick subsystem")
set(SDL_SENSOR OFF CACHE BOOL "Disable SDL sensor subsystem")
set(SDL_POWER OFF CACHE BOOL "Disable SDL power management subsystem")
set(SDL_FILESYSTEM OFF CACHE BOOL "Disable SDL filesystem subsystem")
set(SDL_TIMERS OFF CACHE BOOL "Disable SDL timers subsystem")
set(SDL_3DNOW OFF CACHE BOOL "Disable SDL timers subsystem")
set(SDL_ALTIVEC OFF CACHE BOOL "Disable SDL timers subsystem")
set(SDL_ASSEMBLY OFF CACHE BOOL "Disable SDL timers subsystem")
set(SDL_DUMMYVIDEO OFF CACHE BOOL "Disable SDL timers subsystem")
set(SDL_DUMMYAUDIO OFF CACHE BOOL "Disable SDL timers subsystem")
set(SDL_DISKAUDIO OFF CACHE BOOL "Disable SDL timers subsystem")
set(SDL_DIRECTX OFF CACHE BOOL "Disable SDL timers subsystem")
set(SDL_HIDAPI_JOYSTICK OFF CACHE BOOL "Disable SDL timers subsystem")
set(SDL_OPENGL OFF CACHE BOOL "Disable SDL timers subsystem")
set(SDL_OPENGLES OFF CACHE BOOL "Disable SDL timers subsystem")
set(SDL_RENDER_D3D OFF CACHE BOOL "Disable SDL timers subsystem")
set(SDL_VIRTUAL_JOYSTICK OFF CACHE BOOL "Disable SDL timers subsystem")

# Disable building examples and tests
set(SDL_TEST OFF CACHE BOOL "Do not build SDL test programs")
set(SDL_INSTALL_TESTS OFF CACHE BOOL "Do not install SDL test programs")
set(SDL_ATOMIC OFF CACHE BOOL "Disable SDL atomic operations")

# Build SDL as a static library if you prefer static linking
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build SDL as a static library")

FetchContent_Declare(
  sdl
  URL https://github.com/libsdl-org/SDL/releases/download/release-2.30.7/SDL2-2.30.7.tar.gz
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/sdl
)

FetchContent_MakeAvailable(sdl)

