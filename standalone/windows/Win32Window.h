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
    Win32Window(const std::string& title, int width, int height)
        : m_width(width), m_height(height)
    {
        WNDCLASSEXW wc{};
        wc.cbSize        = sizeof(wc);
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = WndProc;
        wc.hInstance     = GetModuleHandleW(nullptr);
        wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = L"SingularityWindow";
        RegisterClassExW(&wc);

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
            nullptr, nullptr, GetModuleHandleW(nullptr), this
        );
        if (!m_hwnd)
            throw std::runtime_error("Failed to create Win32 window");

        ShowWindow(m_hwnd, SW_SHOW);
        UpdateWindow(m_hwnd);
    }

    ~Win32Window() override {
        if (m_hwnd) DestroyWindow(m_hwnd);
    }

    Win32Window(const Win32Window&) = delete;
    Win32Window& operator=(const Win32Window&) = delete;

    void run() override {
        MSG msg;
        m_running = true;
        while (m_running && GetMessageW(&msg, nullptr, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    void close() override {
        m_running = false;
        PostMessageW(m_hwnd, WM_CLOSE, 0, 0);
    }

    int width()  const override { return m_width; }
    int height() const override { return m_height; }

    void setOnMouseDown(std::function<void(int, int, unsigned int)> cb) override { m_onMouseDown = std::move(cb); }
    void setOnMouseUp(std::function<void(int, int, unsigned int)> cb)   override { m_onMouseUp   = std::move(cb); }

    HWND hwnd() const { return m_hwnd; }

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        Win32Window* win = nullptr;
        if (msg == WM_NCCREATE) {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
            win = static_cast<Win32Window*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(win));
        } else {
            win = reinterpret_cast<Win32Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        if (win) {
            auto fireDown = [&](unsigned int btn) {
                if (win->m_onMouseDown)
                    win->m_onMouseDown(GET_X_LPARAM(lp), GET_Y_LPARAM(lp), btn);
            };
            auto fireUp = [&](unsigned int btn) {
                if (win->m_onMouseUp)
                    win->m_onMouseUp(GET_X_LPARAM(lp), GET_Y_LPARAM(lp), btn);
            };
            switch (msg) {
                case WM_LBUTTONDOWN: fireDown(1); return 0;
                case WM_LBUTTONUP:   fireUp(1);   return 0;
                case WM_MBUTTONDOWN: fireDown(2); return 0;
                case WM_MBUTTONUP:   fireUp(2);   return 0;
                case WM_RBUTTONDOWN: fireDown(3); return 0;
                case WM_RBUTTONUP:   fireUp(3);   return 0;
                case WM_DESTROY:
                    win->m_running = false;
                    PostQuitMessage(0);
                    return 0;
            }
        }
        return DefWindowProcW(hwnd, msg, wp, lp);
    }

    HWND m_hwnd    = nullptr;
    int  m_width   = 0;
    int  m_height  = 0;
    bool m_running = false;
    std::function<void(int, int, unsigned int)> m_onMouseDown;
    std::function<void(int, int, unsigned int)> m_onMouseUp;
};
