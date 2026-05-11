#pragma once

#include "IWindow.h"
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <string>

#if defined(_WIN32)
#  include "windows/Win32Window.h"
#elif defined(__APPLE__)
#  include "macos/CocoaWindow.h"
#else
#  ifdef HAS_WAYLAND
#    include "linux/WaylandWindow.h"
#  endif
#  include "linux/X11Window.h"
#endif

inline std::unique_ptr<IWindow> createNativeWindow(const std::string& title,
                                                    int width, int height,
                                                    void* parentHandle = nullptr,
                                                    IWindow* owner = nullptr)
{
#if defined(_WIN32)
    void* ownerHandle = owner ? owner->nativeHandle() : nullptr;
    bool embedded = (parentHandle != nullptr);
    void* parent = embedded ? parentHandle : ownerHandle;
    return std::make_unique<Win32Window>(title, width, height, parent, embedded);
#elif defined(__APPLE__)
    void* ownerHandle = owner ? owner->nativeHandle() : nullptr;
    return std::make_unique<CocoaWindow>(title, width, height, parentHandle, ownerHandle);
#else
#  ifdef HAS_WAYLAND
    if (std::getenv("WAYLAND_DISPLAY"))
        return std::make_unique<WaylandWindow>(title, width, height);
#  endif
    if (std::getenv("DISPLAY"))
        return std::make_unique<X11Window>(title, width, height);

    throw std::runtime_error(
        "No display server found — set WAYLAND_DISPLAY or DISPLAY");
#endif
}
