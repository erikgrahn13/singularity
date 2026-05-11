#include "WebViewMacos.h"

std::unique_ptr<ISingularityGUI> ISingularityGUI::createView(void *windowHandle, std::function<void()> onReady)
{
    return std::make_unique<WebViewMacos>(windowHandle, onReady);
}

WebViewMacos::WebViewMacos(void *windowHandle, std::function<void()> onReady) : m_windowHandle(windowHandle)
{
    m_viewReadyCallback = onReady;
}

WebViewMacos::~WebViewMacos()
{
}

void WebViewMacos::initialize()
{
    swiftWebView = SingularityView::createWebViewMacos(m_windowHandle, this);
    OnWebViewReady();
}

void WebViewMacos::navigate(const std::string &url)
{
    swiftWebView->navigate(url);
}
