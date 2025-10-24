#include "VST3Window.h"

void *VST3Window::createPlatformWindow(void *parent)
{
    HWND childWindow = CreateWindowEx(0,         // No extended styles
                                      L"STATIC", // Simple window class
                                      L"",       // No title needed
                                      WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0,
                                      0,                           // Position at top-left of parent
                                      PLUGIN_WIDTH, PLUGIN_HEIGHT, // Size you specified in getSize()
                                      (HWND)parent,                // Parent HWND from the host
                                      nullptr, GetModuleHandle(nullptr), nullptr);
    return childWindow;
}