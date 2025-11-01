#include "singularity_Webview_Linux.h"

#include <iostream>
#include <string>

std::unique_ptr<ISingularityGUI> ISingularityGUI::createView(void *windowHandle, std::function<void()> onReady)
{
    return std::make_unique<WebViewLinux>(windowHandle, onReady);
}

WebViewLinux::WebViewLinux(void *windowHandle, std::function<void()> onReady)
    : m_window(static_cast<GtkWidget *>(windowHandle))
{
    m_viewReadyCallback = onReady;
}

WebViewLinux::~WebViewLinux()
{
}

void WebViewLinux::create(int width, int height, const std::string &title)
{
}

void WebViewLinux::navigate(const std::string &url)
{
    webkit_web_view_load_uri(WEBKIT_WEB_VIEW(m_webView), url.c_str());
}

void WebViewLinux::run()
{
}

void WebViewLinux::close()
{
}

void WebViewLinux::resize(int width, int height)
{
}

void WebViewLinux::executeScript(const std::string &script)
{
}

void WebViewLinux::initialize()
{
    m_webView = webkit_web_view_new();
    gtk_window_set_child(GTK_WINDOW(m_window), m_webView);
    OnWebViewReady();
}
