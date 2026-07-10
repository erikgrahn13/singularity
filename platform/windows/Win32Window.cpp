#include "Win32Window.h"

// Helper macros to widen a narrow string literal from a macro
#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)

std::unique_ptr<IWindow> IWindow::createWindow(int width, int height) {
    return std::make_unique<Win32Window>(width, height);
}

std::unique_ptr<IWindow> IWindow::createWindow(int width, int height, void* parentWindow) {
    return std::make_unique<Win32Window>(width, height, parentWindow);
}

Win32Window::Win32Window(int width, int height)
    : m_width(width), m_height(height)
{
    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = GetModuleHandleW(nullptr);
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"SingularityWindow";
    RegisterClassExW(&wc); // no-op if already registered
    RECT rect{0, 0, width, height};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    m_hwnd = CreateWindowExW(
        0, L"SingularityWindow", WIDEN(PLUGIN_NAME),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        nullptr, nullptr, GetModuleHandleW(nullptr), this
    );
    
    if (!m_hwnd)
        throw std::runtime_error("Failed to create Win32 window");
    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);
}

Win32Window::Win32Window(int width, int height, void* parentWindow)
    : m_width(width), m_height(height)
{
    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = GetModuleHandleW(nullptr);
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"SingularityWindow";
    RegisterClassExW(&wc); // no-op if already registered

    m_hwnd = CreateWindowExW(
        0, L"SingularityWindow", nullptr,
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
        0, 0, width, height,
        static_cast<HWND>(parentWindow), nullptr, GetModuleHandleW(nullptr), this
    );

    if (!m_hwnd)
        throw std::runtime_error("Failed to create Win32 window");
}
