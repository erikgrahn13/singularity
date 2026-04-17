#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include "../IWindow.h"
#include <functional>
#include <stdexcept>
#include <string>

class Win32Window : public IWindow {
public:
    Win32Window(const std::string& title, int width, int height, HWND parent = nullptr, bool childMode = false)
        : m_width(width), m_height(height)
    {
        WNDCLASSEXW wc{};
        wc.cbSize        = sizeof(wc);
        wc.lpfnWndProc   = WndProc;
        wc.hInstance     = GetModuleHandleW(nullptr);
        wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = L"SingularityWindow";
        RegisterClassExW(&wc); // no-op if already registered

        if (parent && childMode) {
            // Child window — embedded into host (VST3)
            m_hwnd = CreateWindowExW(
                0, L"SingularityWindow", nullptr,
                WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
                0, 0, width, height,
                parent, nullptr, GetModuleHandleW(nullptr), this
            );
        } else {
            // Top-level window (standalone or owned modal like settings)
            int len = MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, nullptr, 0);
            std::wstring wtitle(len, 0);
            MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, wtitle.data(), len);

            RECT rect{0, 0, width, height};
            AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

            m_hwnd = CreateWindowExW(
                0, L"SingularityWindow", wtitle.c_str(),
                WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT, CW_USEDEFAULT,
                rect.right - rect.left, rect.bottom - rect.top,
                parent, nullptr, GetModuleHandleW(nullptr), this
            );
            ShowWindow(m_hwnd, SW_SHOW);
            UpdateWindow(m_hwnd);
        }

        if (!m_hwnd)
            throw std::runtime_error("Failed to create Win32 window");
    }

    ~Win32Window() override { if (m_hwnd) DestroyWindow(m_hwnd); }

    Win32Window(const Win32Window&) = delete;
    Win32Window& operator=(const Win32Window&) = delete;

    void run() override {
        SetTimer(m_hwnd, 1, 16, nullptr);
        MSG msg{};
        while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        KillTimer(m_hwnd, 1);
    }

    // For secondary windows owned by a running main window — starts the timer
    // without entering a blocking message loop.
    void startTimer() { SetTimer(m_hwnd, 1, 16, nullptr); }
    void stopTimer()  { KillTimer(m_hwnd, 1); }

    void close() override { PostMessageW(m_hwnd, WM_CLOSE, 0, 0); }

    int width()  const override { return m_width; }
    int height() const override { return m_height; }

    void setOnMouseDown(std::function<void(int, int, unsigned int)> cb) override { m_onMouseDown = std::move(cb); }
    void setOnMouseUp(std::function<void(int, int, unsigned int)> cb)   override { m_onMouseUp   = std::move(cb); }
    void setOnMouseMove(std::function<void(int, int)> cb)               override { m_onMouseMove = std::move(cb); }
    void setOnFrame(std::function<DrawingContent()> cb) override { m_onFrame = std::move(cb); startTimer(); }
    void setOnClose(std::function<void()> cb)                           override { m_onClose     = std::move(cb); }

    HWND hwnd() const { return m_hwnd; }

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        Win32Window* win = nullptr;
        if (msg == WM_NCCREATE) {
            win = static_cast<Win32Window*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(win));
        } else {
            win = reinterpret_cast<Win32Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        if (win) {
            switch (msg) {
                case WM_LBUTTONDOWN: if (win->m_onMouseDown) win->m_onMouseDown(GET_X_LPARAM(lp), GET_Y_LPARAM(lp), 1); return 0;
                case WM_LBUTTONUP:   if (win->m_onMouseUp)   win->m_onMouseUp  (GET_X_LPARAM(lp), GET_Y_LPARAM(lp), 1); return 0;
                case WM_RBUTTONDOWN: if (win->m_onMouseDown) win->m_onMouseDown(GET_X_LPARAM(lp), GET_Y_LPARAM(lp), 3); return 0;
                case WM_RBUTTONUP:   if (win->m_onMouseUp)   win->m_onMouseUp  (GET_X_LPARAM(lp), GET_Y_LPARAM(lp), 3); return 0;
                case WM_TIMER:
                    if (win->m_onFrame) {
                        DrawingContent dc = win->m_onFrame();
                        // Null contentAddres means "nothing changed, skip repaint"
                        if (dc.contentAddres) {
                            win->m_frameData = dc;
                            InvalidateRect(hwnd, nullptr, FALSE);
                        }
                    }
                    return 0;
                case WM_MOUSEMOVE:   if (win->m_onMouseMove) win->m_onMouseMove(GET_X_LPARAM(lp), GET_Y_LPARAM(lp)); return 0;
                case WM_PAINT: {
                    PAINTSTRUCT ps;
                    HDC hdc = BeginPaint(hwnd, &ps);
                    const auto& fd = win->m_frameData;
                    if (fd.contentAddres && fd.width > 0 && fd.height > 0) {
                        BITMAPINFO bmi{};
                        bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
                        bmi.bmiHeader.biWidth       = fd.width;
                        bmi.bmiHeader.biHeight      = -fd.height; // top-down
                        bmi.bmiHeader.biPlanes      = 1;
                        bmi.bmiHeader.biBitCount    = 32;
                        bmi.bmiHeader.biCompression = BI_RGB;
                        SetDIBitsToDevice(hdc,
                            0, 0, fd.width, fd.height,
                            0, 0, 0, fd.height,
                            fd.contentAddres, &bmi, DIB_RGB_COLORS);
                    }
                    EndPaint(hwnd, &ps);
                    return 0;
                }
                case WM_ERASEBKGND: return 1;
                case WM_DESTROY:
                    win->m_hwnd = nullptr; // prevent destructor double-destroy
                    if (win->m_onClose) win->m_onClose();
                    if (!GetWindow(hwnd, GW_OWNER)) PostQuitMessage(0);
                    return 0;
            }
        }
        return DefWindowProcW(hwnd, msg, wp, lp);
    }

    HWND m_hwnd  = nullptr;
    int  m_width = 0;
    int  m_height = 0;
    DrawingContent m_frameData{};
    std::function<void(int, int, unsigned int)> m_onMouseDown;
    std::function<void(int, int, unsigned int)> m_onMouseUp;
    std::function<void(int, int)>               m_onMouseMove;
    std::function<DrawingContent()>             m_onFrame;
    std::function<void()>                       m_onClose;
};
