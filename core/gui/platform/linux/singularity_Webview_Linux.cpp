#include "singularity_Webview_Linux.h"

#include <X11/Xlib.h>
#include <glib-unix.h>
#include <gtk/gtk.h>
#include <gtk/gtkx.h>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <webkit2/webkit2.h>

std::unique_ptr<ISingularityGUI> ISingularityGUI::createView(void *windowHandle, std::function<void()> onReady)
{
    return std::make_unique<WebViewLinux>(windowHandle, onReady);
}

WebViewLinux::WebViewLinux(void *windowHandle, std::function<void()> onReady) : m_window(windowHandle)
{
    m_viewReadyCallback = onReady;
}

WebViewLinux::~WebViewLinux()
{
    if (m_childPid > 0)
    {
        // Send quit command to child
        if (m_pipeToChild[1] != -1)
        {
            std::string cmd = "Q\n";
            write(m_pipeToChild[1], cmd.c_str(), cmd.length());
            ::close(m_pipeToChild[1]);
            m_pipeToChild[1] = -1;
        }

        // Wait for child to exit (with timeout)
        int status;
        pid_t result = waitpid(m_childPid, &status, WNOHANG);
        if (result == 0)
        {
            // Child hasn't exited yet, give it a moment
            usleep(50000); // 50ms
            result = waitpid(m_childPid, &status, WNOHANG);
        }

        if (result == 0)
        {
            // Still not exited, force kill
            kill(m_childPid, SIGKILL);
            waitpid(m_childPid, &status, 0);
        }

        m_childPid = 0;
    }
}

void WebViewLinux::create(int width, int height, const std::string &title)
{
}

void WebViewLinux::navigate(const std::string &url)
{
    if (m_pipeToChild[1] != -1)
    {
        // Send navigate command: "N" + url + "\n"
        std::string cmd = "N" + url + "\n";
        write(m_pipeToChild[1], cmd.c_str(), cmd.length());
    }
}

void WebViewLinux::run()
{
}

void WebViewLinux::close()
{
}

void WebViewLinux::resize(int width, int height)
{
    if (m_pipeToChild[1] != -1)
    {
        // Send resize command: "R" + width + "x" + height + "\n"
        std::string cmd = "R" + std::to_string(width) + "x" + std::to_string(height) + "\n";
        write(m_pipeToChild[1], cmd.c_str(), cmd.length());
    }
}

void WebViewLinux::executeScript(const std::string &script)
{
    if (m_pipeToChild[1] != -1)
    {
        // Send script command: "S" + script + "\n"
        std::string cmd = "S" + script + "\n";
        write(m_pipeToChild[1], cmd.c_str(), cmd.length());
    }
}

static void childProcessMain(void *parentWindow, int pipeRead, int pipeWrite)
{
    gtk_init(nullptr, nullptr);

    GtkWidget *topLevel;

    if (parentWindow != nullptr)
    {
        // VST3 mode: Embed into existing window
        unsigned long parentWindowId = (unsigned long)parentWindow;
        topLevel = gtk_plug_new(parentWindowId);
        gtk_widget_set_size_request(topLevel, 800, 600);
    }
    else
    {
        // Standalone mode: Create independent window
        topLevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(topLevel), "Singularity");
        gtk_window_set_default_size(GTK_WINDOW(topLevel), 800, 600);
    }

    // Same WebView code for both modes
    WebKitSettings *settings = webkit_settings_new();
    webkit_settings_set_hardware_acceleration_policy(settings, WEBKIT_HARDWARE_ACCELERATION_POLICY_NEVER);
    GtkWidget *webView = webkit_web_view_new_with_settings(settings);

    gtk_container_add(GTK_CONTAINER(topLevel), webView);
    gtk_widget_show_all(topLevel);

    // VST3-specific X11 resize
    if (parentWindow != nullptr)
    {
        gtk_widget_realize(topLevel);
        unsigned long windowId = gtk_plug_get_id(GTK_PLUG(topLevel));
        Display *display = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
        if (display)
        {
            XResizeWindow(display, windowId, 800, 600);
            XMapWindow(display, windowId);
            XFlush(display);
        }
    }

    // Monitor pipe for commands
    g_unix_fd_add(
        pipeRead, (GIOCondition)(G_IO_IN | G_IO_HUP),
        [](gint fd, GIOCondition condition, gpointer user_data) -> gboolean {
            // Check if pipe was closed (parent exited)
            if (condition & G_IO_HUP)
            {
                gtk_main_quit();
                return G_SOURCE_REMOVE;
            }

            auto *webView = static_cast<GtkWidget *>(user_data);

            char buffer[4096];
            ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
            if (n <= 0)
            {
                gtk_main_quit();
                return G_SOURCE_REMOVE;
            }

            buffer[n] = '\0';
            std::string cmd(buffer);

            if (cmd[0] == 'N')
            {
                // Navigate
                std::string url = cmd.substr(1);
                url.erase(url.find_last_not_of("\n") + 1);
                webkit_web_view_load_uri(WEBKIT_WEB_VIEW(webView), url.c_str());
            }
            else if (cmd[0] == 'S')
            {
                // Execute script
                std::string script = cmd.substr(1);
                script.erase(script.find_last_not_of("\n") + 1);
                webkit_web_view_run_javascript(WEBKIT_WEB_VIEW(webView), script.c_str(), nullptr, nullptr, nullptr);
            }
            else if (cmd[0] == 'R')
            {
                // Resize
                std::string sizeStr = cmd.substr(1);
                sizeStr.erase(sizeStr.find_last_not_of("\n") + 1);
                size_t xPos = sizeStr.find('x');
                if (xPos != std::string::npos)
                {
                    int width = std::stoi(sizeStr.substr(0, xPos));
                    int height = std::stoi(sizeStr.substr(xPos + 1));

                    Display *display = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
                    GtkWidget *topLevel = gtk_widget_get_toplevel(webView);

                    if (GTK_IS_PLUG(topLevel))
                    {
                        unsigned long windowId = gtk_plug_get_id(GTK_PLUG(topLevel));
                        if (display && windowId)
                        {
                            XResizeWindow(display, windowId, width, height);
                            XFlush(display);
                        }
                    }
                    else if (GTK_IS_WINDOW(topLevel))
                    {
                        gtk_window_resize(GTK_WINDOW(topLevel), width, height);
                    }
                }
            }
            else if (cmd[0] == 'Q')
            {
                // Quit command
                gtk_main_quit();
                return G_SOURCE_REMOVE;
            }

            return G_SOURCE_CONTINUE;
        },
        webView);

    // Signal parent that we're ready (AFTER pipe monitoring is set up)
    unsigned long readySignal = 1;
    write(pipeWrite, &readySignal, sizeof(readySignal));
    ::close(pipeWrite); // Done signaling, close write end

    // Run GTK main loop
    gtk_main();

    exit(0);
}

void WebViewLinux::initialize()
{
    // Create pipe for parent->child communication
    if (pipe(m_pipeToChild) == -1)
    {
        return;
    }

    // Fork child process
    m_childPid = fork();

    if (m_childPid == -1)
    {
        return;
    }

    if (m_childPid == 0)
    {
        // CHILD PROCESS
        childProcessMain(m_window, m_pipeToChild[0], m_pipeToChild[1]);

        // Never reaches here
        exit(0);
    }

    // PARENT PROCESS
    // Wait for child to signal it's ready (blocking read)
    unsigned long readySignal = 0;
    ssize_t bytesRead = read(m_pipeToChild[0], &readySignal, sizeof(readySignal));

    if (bytesRead != sizeof(readySignal))
    {
        ::close(m_pipeToChild[0]);
        ::close(m_pipeToChild[1]);
        return;
    }

    ::close(m_pipeToChild[0]); // Close read end, only keep write end for commands

    // Call ready callback
    OnWebViewReady();
}
