#include "X11Window.h"
#include <cmath>
#include <poll.h>
#include <libportal/portal.h>
#include <glib.h>

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
    int x11_fd = ConnectionNumber(display_);
    int frame_interval_ms = 1000 / refreshRate();

    while (true)
    {
        // Sleep until next frame or X11 event
        struct pollfd pfd = { x11_fd, POLLIN, 0 };
        poll(&pfd, 1, frame_interval_ms);

        // Drain all pending X events
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
                    break;
                if (onMouseUp_) onMouseUp_(event.xbutton.x, event.xbutton.y);
                break;
            case MotionNotify:
                while (XPending(display_)) {
                    XEvent next;
                    XPeekEvent(display_, &next);
                    if (next.type != MotionNotify) break;
                    XNextEvent(display_, &event);
                }
                if (onMouseMove_) onMouseMove_(event.xmotion.x, event.xmotion.y);
                break;
            default:
                break;
            }
        }

        if (onFrame_) onFrame_();

        while (g_main_context_iteration(nullptr, FALSE)) {}
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
    // Pump GLib main context for async portal callbacks
    while (g_main_context_iteration(nullptr, FALSE)) {}

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

void X11Window::openFileDialog(const std::string& title,
                                std::function<void(const std::string&)> callback)
{
    XdpPortal* portal = xdp_portal_new();
    auto* cb = new std::function<void(const std::string&)>(std::move(callback));

    // Build filter: "WAV Files" matching *.wav
    GVariantBuilder patternBuilder;
    g_variant_builder_init(&patternBuilder, G_VARIANT_TYPE("a(us)"));
    g_variant_builder_add(&patternBuilder, "(us)", 0, "*.wav");

    GVariantBuilder filterBuilder;
    g_variant_builder_init(&filterBuilder, G_VARIANT_TYPE("a(sa(us))"));
    g_variant_builder_add(&filterBuilder, "(sa(us))", "WAV Files", &patternBuilder);

    GVariant* filters = g_variant_builder_end(&filterBuilder);

    xdp_portal_open_file(
        portal,
        nullptr,                      // parent window
        title.c_str(),                // title
        filters,                      // filters
        nullptr,                      // current_filter
        nullptr,                      // current_folder
        XDP_OPEN_FILE_FLAG_NONE,      // flags
        nullptr,                      // GCancellable
        [](GObject* obj, GAsyncResult* res, gpointer data) {
            auto* userCb = static_cast<std::function<void(const std::string&)>*>(data);
            GError* error = nullptr;
            GVariant* result = xdp_portal_open_file_finish(XDP_PORTAL(obj), res, &error);

            std::string selectedPath;
            if (result) {
                const char** uris = nullptr;
                g_variant_lookup(result, "uris", "^a&s", &uris);
                if (uris && uris[0]) {
                    GFile* file = g_file_new_for_uri(uris[0]);
                    char* path = g_file_get_path(file);
                    if (path) {
                        selectedPath = path;
                        g_free(path);
                    }
                    g_object_unref(file);
                }
                g_variant_unref(result);
            }
            if (error) g_error_free(error);

            if (*userCb) (*userCb)(selectedPath);
            delete userCb;
            g_object_unref(obj);
        },
        cb
    );
}
