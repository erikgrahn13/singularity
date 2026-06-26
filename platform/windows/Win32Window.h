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
    Win32Window(int width, int height);
    Win32Window(int width, int height, void* parent);
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

    void resize(int w, int h) override {
        m_width = w; m_height = h;
        if (m_hwnd) SetWindowPos(m_hwnd, nullptr, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER);
    }

    void setOnMouseDown(std::function<void(int, int)> cb) override { m_onMouseDown = std::move(cb); }
    void setOnMouseUp(std::function<void(int, int)> cb)   override { m_onMouseUp   = std::move(cb); }
    void setOnMouseMove(std::function<void(int, int)> cb)               override { m_onMouseMove = std::move(cb); }
    void setOnFrame(std::function<void()> cb) override { m_onFrame = std::move(cb); startTimer(); }
    void setOnClose(std::function<void()> cb)                           override { m_onClose     = std::move(cb); }

    HWND hwnd() const { return m_hwnd; }
    void* nativeHandle() const override { return m_hwnd; }

    void setResizable(bool resizable) override {
        if (!m_hwnd) return;
        LONG style = GetWindowLongW(m_hwnd, GWL_STYLE);
        if (resizable) {
            style |= WS_THICKFRAME | WS_MAXIMIZEBOX;
        } else {
            style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
        }
        SetWindowLongW(m_hwnd, GWL_STYLE, style);
        SetWindowPos(m_hwnd, nullptr, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }

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
                case WM_LBUTTONDOWN: if (win->m_onMouseDown) win->m_onMouseDown(GET_X_LPARAM(lp), GET_Y_LPARAM(lp)); return 0;
                case WM_LBUTTONUP:   if (win->m_onMouseUp)   win->m_onMouseUp  (GET_X_LPARAM(lp), GET_Y_LPARAM(lp)); return 0;
                case WM_RBUTTONDOWN: if (win->m_onMouseDown) win->m_onMouseDown(GET_X_LPARAM(lp), GET_Y_LPARAM(lp)); return 0;
                case WM_RBUTTONUP:   if (win->m_onMouseUp)   win->m_onMouseUp  (GET_X_LPARAM(lp), GET_Y_LPARAM(lp)); return 0;
                case WM_TIMER:
                    if (win->m_onFrame) {
                        win->m_onFrame();
                    }
                    return 0;
                case WM_MOUSEMOVE:   if (win->m_onMouseMove) win->m_onMouseMove(GET_X_LPARAM(lp), GET_Y_LPARAM(lp)); return 0;
                case WM_PAINT: {
                    PAINTSTRUCT ps;
                    BeginPaint(hwnd, &ps);
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
    std::function<void(int, int)> m_onMouseDown;
    std::function<void(int, int)> m_onMouseUp;
    std::function<void(int, int)>               m_onMouseMove;
    std::function<void()>                       m_onFrame;
    std::function<void()>                       m_onClose;
};
