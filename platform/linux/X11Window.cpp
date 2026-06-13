#include "X11Window.h"
#include <cmath>

std::unique_ptr<IWindow> IWindow::createWindow(int width, int height, void* parentWindow)
{
    return std::make_unique<X11Window>(width, height, parentWindow);
}

X11Window::X11Window(int width, int height, void* parentWindow) : width_(width), height_(height)
{
        display_ = XOpenDisplay(NULL);
        if(!display_)
        {
            throw std::runtime_error("Failed to open X11 display");
        }

        // Screen* screen = DefaultScreenOfDisplay(display);
        int screenIdl = DefaultScreen(display_);

        Window rootWindow = parentWindow ? (Window)(uintptr_t)parentWindow : DefaultRootWindow(display_);
        window_ = XCreateSimpleWindow(display_, rootWindow, 0, 0, width, height, 0, 0, 0);

        XSelectInput(display_, window_, ButtonPressMask | ButtonReleaseMask | PointerMotionMask | StructureNotifyMask);

        if (parentWindow) {
            Atom xembedInfo = XInternAtom(display_, "_XEMBED_INFO", False);
            unsigned long info[2] = { 0, 1 }; // version=0, flags=XEMBED_MAPPED
            XChangeProperty(display_, window_, xembedInfo, xembedInfo, 32,
                            PropModeReplace, (unsigned char*)info, 2);
        }

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
        // Drain all pending X events first
        while (XPending(display_))
        {
            XNextEvent(display_, &event);

            switch (event.type)
            {
            case ButtonPress:
                if (event.xbutton.button >= 4 && event.xbutton.button <= 7) {
                    if (onMouseWheel_) {
                        float dx = (event.xbutton.button == 6) ? -1.0f : (event.xbutton.button == 7) ? 1.0f : 0.0f;
                        float dy = (event.xbutton.button == 4) ? 1.0f : (event.xbutton.button == 5) ? -1.0f : 0.0f;
                        onMouseWheel_(dx, dy);
                    }
                } else {
                    if (onMouseDown_) onMouseDown_(event.xbutton.x, event.xbutton.y);
                }
                break;
            case ButtonRelease:
                if (event.xbutton.button >= 4 && event.xbutton.button <= 7)
                    break; // ignore wheel release events
                if (onMouseUp_) onMouseUp_(event.xbutton.x, event.xbutton.y);
                break;
            case MotionNotify:
                if (onMouseMove_) onMouseMove_(event.xmotion.x, event.xmotion.y);
                break;
            default:
                break;
            }
        }

        // Render frame — vkQueuePresentKHR with FIFO present mode acts as vsync
        if (onFrame_) onFrame_();
    }
}

void X11Window::resize(int w, int h) {
    width_ = w; height_ = h;
    XResizeWindow(display_, window_, w, h);
    XFlush(display_);
}

void X11Window::setResizable(bool resizable) {
    XSizeHints* hints = XAllocSizeHints();
    if (resizable) {
        hints->flags = PMinSize;
        hints->min_width = 1;
        hints->min_height = 1;
    } else {
        hints->flags = PMinSize | PMaxSize;
        hints->min_width = width_;
        hints->min_height = height_;
        hints->max_width = width_;
        hints->max_height = height_;
    }
    XSetWMNormalHints(display_, window_, hints);
    XFree(hints);
    XFlush(display_);
}

void X11Window::processEvents()
{
    XEvent event;
    while (XPending(display_)) {
        XNextEvent(display_, &event);
        switch (event.type) {
        case ButtonPress:
            if (event.xbutton.button >= 4 && event.xbutton.button <= 7) {
                if (onMouseWheel_) {
                    float dx = (event.xbutton.button == 6) ? -1.0f : (event.xbutton.button == 7) ? 1.0f : 0.0f;
                    float dy = (event.xbutton.button == 4) ? 1.0f : (event.xbutton.button == 5) ? -1.0f : 0.0f;
                    onMouseWheel_(dx, dy);
                }
            } else {
                if (onMouseDown_) onMouseDown_(event.xbutton.x, event.xbutton.y);
            }
            break;
        case ButtonRelease:
            if (event.xbutton.button >= 4 && event.xbutton.button <= 7)
                break;
            if (onMouseUp_) onMouseUp_(event.xbutton.x, event.xbutton.y);
            break;
        case MotionNotify:
            if (onMouseMove_) onMouseMove_(event.xmotion.x, event.xmotion.y);
            break;
        default:
            break;
        }
    }
}

int X11Window::refreshRate() const {
    int rate = 60; // fallback
    XRRScreenResources* res = XRRGetScreenResources(display_, DefaultRootWindow(display_));
    if (!res) return rate;

    for (int i = 0; i < res->ncrtc; ++i) {
        XRRCrtcInfo* crtc = XRRGetCrtcInfo(display_, res, res->crtcs[i]);
        if (!crtc || crtc->mode == None) {
            XRRFreeCrtcInfo(crtc);
            continue;
        }
        for (int j = 0; j < res->nmode; ++j) {
            if (res->modes[j].id == crtc->mode) {
                auto& m = res->modes[j];
                if (m.hTotal && m.vTotal)
                    rate = std::max(rate, (int)std::round((double)m.dotClock / (m.hTotal * m.vTotal)));
                break;
            }
        }
        XRRFreeCrtcInfo(crtc);
    }
    XRRFreeScreenResources(res);
    return rate;
}
