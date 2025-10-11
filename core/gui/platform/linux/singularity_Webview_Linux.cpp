#include "webview_linux.h"

#ifdef __linux__
#include "../../resource_manager.h"
#include <gtk/gtk.h>
#include <iostream>
#include <string>
#include <webkit2/webkit2.h>

// Custom URI scheme handler for serving embedded resources
static void app_scheme_handler(WebKitURISchemeRequest *request, gpointer user_data)
{
    const gchar *uri = webkit_uri_scheme_request_get_uri(request);
    std::string uriStr(uri);

    // Extract path from URI (remove "app://localhost")
    std::string path = "/";
    size_t pathStart = uriStr.find("app://localhost");
    if (pathStart != std::string::npos)
    {
        pathStart += 15; // Length of "app://localhost"
        if (pathStart < uriStr.length())
        {
            path = uriStr.substr(pathStart);
        }
    }

    // Handle root path
    if (path.empty() || path == "/")
    {
        path = "/index.html";
    }

    // Get resource from embedded data
    auto resource = ResourceManager::getResource(path);
    if (resource)
    {
        // Create input stream from memory
        GInputStream *stream = g_memory_input_stream_new_from_data(resource->data, resource->size, nullptr);

        // Create response
        webkit_uri_scheme_request_finish(request, stream, resource->size, resource->mimeType.c_str());

        g_object_unref(stream);
    }
    else
    {
        // Resource not found
        GError *error = g_error_new(G_IO_ERROR, G_IO_ERROR_NOT_FOUND, "Resource not found: %s", path.c_str());
        webkit_uri_scheme_request_finish_error(request, error);
        g_error_free(error);
    }
}

WebViewLinux::WebViewLinux() : m_window(nullptr), m_webView(nullptr)
{
}

WebViewLinux::~WebViewLinux()
{
    close();
}

void WebViewLinux::create(int width, int height, const std::string &title)
{
    // Initialize GTK
    gtk_init(nullptr, nullptr);

    // Create window
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), title.c_str());
    gtk_window_set_default_size(GTK_WINDOW(window), width, height);

    // Connect close signal
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), nullptr);

    // Register custom URI scheme for embedded resources
    WebKitWebContext *context = webkit_web_context_get_default();
    webkit_web_context_register_uri_scheme(context, "app", app_scheme_handler, nullptr, nullptr);

    // Create WebView
    GtkWidget *webView = webkit_web_view_new_with_context(context);
    gtk_container_add(GTK_CONTAINER(window), webView);

    m_window = window;
    m_webView = webView;

    gtk_widget_show_all(window);
}

void WebViewLinux::navigate(const std::string &url)
{
    if (!m_webView)
        return;
    webkit_web_view_load_uri(WEBKIT_WEB_VIEW(m_webView), url.c_str());
}

void WebViewLinux::run()
{
    gtk_main();
}

void WebViewLinux::close()
{
    if (m_window)
    {
        gtk_widget_destroy(GTK_WIDGET(m_window));
        m_window = nullptr;
        m_webView = nullptr;
    }
}

#else
// Stub implementation for non-Linux platforms
WebViewLinux::WebViewLinux() : m_window(nullptr), m_webView(nullptr)
{
}
WebViewLinux::~WebViewLinux()
{
}
void WebViewLinux::create(int width, int height, const std::string &title)
{
}
void WebViewLinux::navigate(const std::string &url)
{
}
void WebViewLinux::run()
{
}
void WebViewLinux::close()
{
}
#endif
