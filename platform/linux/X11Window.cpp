#include "X11Window.h"

std::unique_ptr<IWindow> IWindow::createWindow(int width, int height)
{
    return std::make_unique<X11Window>(width, height);
}

X11Window::X11Window(int width, int height) : width_(width), height_(height)
{
        display_ = XOpenDisplay(NULL);
        if(!display_)
        {
            throw std::runtime_error("Failed to open X11 display");
        }

        // Screen* screen = DefaultScreenOfDisplay(display);
        int screenIdl = DefaultScreen(display_);

        window_ = XCreateSimpleWindow(display_, DefaultRootWindow(display_), 0, 0, width, height, 0, 0, 0);

        XSelectInput(display_, window_, ButtonPressMask | ButtonReleaseMask | StructureNotifyMask);

        XClearWindow(display_, window_);
        XMapRaised(display_, window_);
}

X11Window::~X11Window()
{
    XDestroyWindow(display_, window_);
    // XFree(screen);
    XCloseDisplay(display_);
}

void X11Window::run()
{
    XEvent event;
    while (true)
    {
        if (onFrame_) onFrame_();

        while (XPending(display_))
        {
            XNextEvent(display_, &event);

            switch (event.type)
            {
            case ButtonPress:
                std::cout << "Mouse clicked" << std::endl;
                break;

            default:
                break;
            }
        }
    }
}

void X11Window::resize(int w, int h) {
    width_ = w; height_ = h;
    XResizeWindow(display_, window_, w, h);
    XFlush(display_);
}

