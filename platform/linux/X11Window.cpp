#include "X11Window.h"
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <spawn.h>
#include <string_view>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

extern char** environ;

namespace
{
bool executableAvailable(std::string_view executable)
{
    const auto* pathValue = std::getenv("PATH");
    if (!pathValue)
        return false;

    std::string_view path{pathValue};
    while (true)
    {
        const auto separator = path.find(':');
        const auto directory = path.substr(0, separator);
        std::string candidate = directory.empty() ? "." : std::string(directory);
        candidate += '/';
        candidate += executable;
        if (::access(candidate.c_str(), X_OK) == 0)
            return true;
        if (separator == std::string_view::npos)
            break;
        path.remove_prefix(separator + 1);
    }
    return false;
}

bool isKdeSession()
{
    if (const auto* fullSession = std::getenv("KDE_FULL_SESSION"))
        if (std::string_view(fullSession) == "true")
            return true;
    if (const auto* desktop = std::getenv("XDG_CURRENT_DESKTOP"))
        return std::string_view(desktop).find("KDE") != std::string_view::npos;
    return false;
}

std::vector<std::string> childEnvironment(Window parentWindow)
{
    std::vector<std::string> result;
    for (auto entry = environ; entry && *entry; ++entry)
    {
        const std::string_view value{*entry};
        // Bundled DAWs can override this with private libraries that external
        // desktop helpers must not inherit.
        if (value.starts_with("LD_LIBRARY_PATH=") ||
            value.starts_with("WINDOWID="))
            continue;
        result.emplace_back(value);
    }
    result.emplace_back("WINDOWID=" + std::to_string(parentWindow));
    return result;
}
}

std::unique_ptr<IWindow> IWindow::createWindow(int width, int height)
{
    return std::make_unique<X11Window>(width, height);
}

std::unique_ptr<IWindow> IWindow::createWindow(int width, int height, void* parentWindow)
{
    return std::make_unique<X11Window>(width, height, parentWindow);
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

        Window rootWindow = DefaultRootWindow(display_);
        window_ = XCreateSimpleWindow(display_, rootWindow, 0, 0, width, height, 0, 0, 0);
        XStoreName(display_, window_, PLUGIN_NAME);

        XSelectInput(display_, window_, ButtonPressMask | ButtonReleaseMask | PointerMotionMask | StructureNotifyMask);

        XClearWindow(display_, window_);
        XMapRaised(display_, window_);
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

        Window rootWindow = (Window)(uintptr_t)parentWindow;
        window_ = XCreateSimpleWindow(display_, rootWindow, 0, 0, width, height, 0, 0, 0);

        XSelectInput(display_, window_, ButtonPressMask | ButtonReleaseMask | PointerMotionMask | StructureNotifyMask);

        Atom xembedInfo = XInternAtom(display_, "_XEMBED_INFO", False);
        unsigned long info[2] = { 0, 1 }; // version=0, flags=XEMBED_MAPPED
        XChangeProperty(display_, window_, xembedInfo, xembedInfo, 32,
                        PropModeReplace, (unsigned char*)info, 2);

        XClearWindow(display_, window_);
        XMapWindow(display_, window_);
}

X11Window::~X11Window()
{
    cancelChooser();
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
        pollChooser();
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
    pollChooser();

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
    launchChooser(title, false, std::move(callback));
}

void X11Window::openDirectoryDialog(const std::string& title,
                                    std::function<void(const std::string&)> callback)
{
    launchChooser(title, true, std::move(callback));
}

void X11Window::launchChooser(
    const std::string& title,
    bool selectDirectory,
    std::function<void(const std::string&)> callback)
{
    if (chooserProcess_)
    {
        std::cerr << "[Singularity] A Linux file chooser is already open\n";
        if (callback)
            callback({});
        return;
    }

    const bool hasZenity = executableAvailable("zenity");
    const bool hasKDialog = executableAvailable("kdialog");
    const bool useKDialog = hasKDialog && (isKdeSession() || !hasZenity);

    std::vector<std::string> arguments;
    if (useKDialog)
    {
        arguments = {
            "kdialog",
            "--title", title,
            "--attach", std::to_string(window_),
            selectDirectory ? "--getexistingdirectory" : "--getopenfilename",
            "",
        };
        if (!selectDirectory)
            arguments.emplace_back("*.wav|WAV Files");
    }
    else if (hasZenity)
    {
        arguments = {
            "zenity",
            "--file-selection",
            "--title=" + title,
        };
        if (selectDirectory)
            arguments.emplace_back("--directory");
        else
            arguments.emplace_back("--file-filter=WAV Files | *.wav");
    }
    else
    {
        std::cerr << "[Singularity] No Linux file chooser found. "
                     "Install zenity or kdialog.\n";
        if (callback)
            callback({});
        return;
    }

    int outputPipe[2] = {-1, -1};
    if (::pipe(outputPipe) != 0)
    {
        std::cerr << "[Singularity] Could not create file chooser pipe: "
                  << std::strerror(errno) << '\n';
        if (callback)
            callback({});
        return;
    }

    posix_spawn_file_actions_t actions;
    if (posix_spawn_file_actions_init(&actions) != 0)
    {
        ::close(outputPipe[0]);
        ::close(outputPipe[1]);
        if (callback)
            callback({});
        return;
    }
    posix_spawn_file_actions_adddup2(&actions, outputPipe[1], STDOUT_FILENO);
    posix_spawn_file_actions_addclose(&actions, outputPipe[0]);
    posix_spawn_file_actions_addclose(&actions, outputPipe[1]);

    std::vector<char*> argv;
    argv.reserve(arguments.size() + 1);
    for (auto& argument : arguments)
        argv.push_back(argument.data());
    argv.push_back(nullptr);

    auto environment = childEnvironment(window_);
    std::vector<char*> envp;
    envp.reserve(environment.size() + 1);
    for (auto& entry : environment)
        envp.push_back(entry.data());
    envp.push_back(nullptr);

    pid_t pid = -1;
    const auto spawnResult = posix_spawnp(
        &pid, argv.front(), &actions, nullptr, argv.data(), envp.data());
    posix_spawn_file_actions_destroy(&actions);
    ::close(outputPipe[1]);

    if (spawnResult != 0)
    {
        ::close(outputPipe[0]);
        std::cerr << "[Singularity] Could not launch Linux file chooser: "
                  << std::strerror(spawnResult) << '\n';
        if (callback)
            callback({});
        return;
    }

    const auto flags = ::fcntl(outputPipe[0], F_GETFL, 0);
    if (flags >= 0)
        ::fcntl(outputPipe[0], F_SETFL, flags | O_NONBLOCK);
    chooserProcess_.emplace(ChooserProcess{
        .pid = pid,
        .outputFd = outputPipe[0],
        .callback = std::move(callback),
    });
}

void X11Window::pollChooser()
{
    if (!chooserProcess_)
        return;

    auto& chooser = *chooserProcess_;
    char buffer[1024];
    while (true)
    {
        const auto count = ::read(chooser.outputFd, buffer, sizeof(buffer));
        if (count > 0)
        {
            chooser.output.append(buffer, static_cast<std::size_t>(count));
            continue;
        }
        if (count < 0 && errno == EINTR)
            continue;
        break;
    }

    int status = 0;
    const auto waitResult = ::waitpid(chooser.pid, &status, WNOHANG);
    if (waitResult == 0 || (waitResult < 0 && errno == EINTR))
        return;

    while (!chooser.output.empty() &&
           (chooser.output.back() == '\n' || chooser.output.back() == '\r'))
        chooser.output.pop_back();
    const bool succeeded = waitResult == chooser.pid && WIFEXITED(status) &&
        WEXITSTATUS(status) == 0;
    auto callback = std::move(chooser.callback);
    auto selectedPath = succeeded ? std::move(chooser.output) : std::string{};
    ::close(chooser.outputFd);
    chooserProcess_.reset();
    if (callback)
        callback(selectedPath);
}

void X11Window::cancelChooser()
{
    if (!chooserProcess_)
        return;
    ::kill(chooserProcess_->pid, SIGTERM);
    while (::waitpid(chooserProcess_->pid, nullptr, 0) < 0 && errno == EINTR)
    {
    }
    ::close(chooserProcess_->outputFd);
    chooserProcess_.reset();
}
