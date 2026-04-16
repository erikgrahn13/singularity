#pragma once

#include <X11/Xlib.h>
#include <functional>
#include <stdexcept>
#include <string>
#include <iostream>
#include "../IWindow.h"

class X11Window : public IWindow {
public:
    X11Window(const std::string& title = "Window", int width = 800, int height = 600)
        : m_width(width), m_height(height)
    {
        m_display = XOpenDisplay(nullptr);
        if (!m_display)
            throw std::runtime_error("Failed to open X11 display");

        int screen = DefaultScreen(m_display);

        m_window = XCreateSimpleWindow(
            m_display,
            RootWindow(m_display, screen),
            0, 0,
            static_cast<unsigned>(width), static_cast<unsigned>(height),
            1,
            BlackPixel(m_display, screen),
            WhitePixel(m_display, screen)
        );

        XStoreName(m_display, m_window, title.c_str());

        // Request window close events via WM_DELETE_WINDOW
        m_wmDeleteMessage = XInternAtom(m_display, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(m_display, m_window, &m_wmDeleteMessage, 1);

        XSelectInput(m_display, m_window, ExposureMask | ButtonPressMask | ButtonReleaseMask | StructureNotifyMask);
        XMapWindow(m_display, m_window);
        XFlush(m_display);
    }

    ~X11Window() {
        if (m_display) {
            XDestroyWindow(m_display, m_window);
            XCloseDisplay(m_display);
        }
    }

    X11Window(const X11Window&) = delete;
    X11Window& operator=(const X11Window&) = delete;

    // Run the event loop until the window is closed.
    void run() override {
        XEvent event;
        m_running = true;
        while (m_running) {
            XNextEvent(m_display, &event);
            switch (event.type) {
                case ClientMessage:
                    if (static_cast<Atom>(event.xclient.data.l[0]) == m_wmDeleteMessage)
                        m_running = false;
                    break;
                case ButtonPress:
                    // if (m_onMouseDown)
                    std::cout << "OnMouseDown" << std::endl;
                        // m_onMouseDown(event.xbutton.x, event.xbutton.y, event.xbutton.button);
                    break;
                case ButtonRelease:
                    // if (m_onMouseUp)
                    std::cout << "OnMouseUp" << std::endl;
                        // m_onMouseUp(event.xbutton.x, event.xbutton.y, event.xbutton.button);
                    break;
                default:
                    break;
            }
        }
    }

    void close()  override { m_running = false; }
    int  width()  const override { return m_width; }
    int  height() const override { return m_height; }

    void setOnMouseDown(std::function<void(int x, int y, unsigned int button)> cb) override { m_onMouseDown = std::move(cb); }
    void setOnMouseUp(std::function<void(int x, int y, unsigned int button)> cb)   override { m_onMouseUp   = std::move(cb); }

    Display* display() const { return m_display; }
    Window   window()  const { return m_window; }

private:
    Display* m_display   = nullptr;
    Window   m_window    = 0;
    Atom     m_wmDeleteMessage{};
    int      m_width     = 0;
    int      m_height    = 0;
    bool     m_running   = false;
    std::function<void(int, int, unsigned int)> m_onMouseDown;
    std::function<void(int, int, unsigned int)> m_onMouseUp;
};
