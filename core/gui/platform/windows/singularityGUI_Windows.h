#pragma once

// #ifdef _WIN32
#include "../../singularity_Webview.h"
#include "WebView2.h"
#include <Shlwapi.h>
#include <string>
#include <wil/com.h>
#include <windows.h>
#include <wrl.h>

using namespace Microsoft::WRL;

// static wil::com_ptr<ICoreWebView2Controller> webviewController;
// static wil::com_ptr<ICoreWebView2Environment> environment;
// static wil::com_ptr<ICoreWebView2> webview;

EventRegistrationToken m_webResourceRequestedToken = {};

class WebViewWindows : public ISingularityGUI
{
  public:
    WebViewWindows(void *windowHandle = nullptr, std::function<void()> onReady = nullptr);
    ~WebViewWindows();

    // ISingularityGUI interface (standalone + child window support)
    void create(int width, int height, const std::string &title) override;
    void createAsChild(void *parentWindow, int width, int height) {};
    void navigate(const std::string &url) override;
    void run() override;
    void close() override;
    void resize(int width, int height) override;
    void executeScript(const std::string &script);
    void initialize() override;

  private:
    HWND m_hwnd;
    // ComPtr<ICoreWebView2Environment> m_environment;
    // ComPtr<ICoreWebView2Controller> m_controller;
    // ComPtr<ICoreWebView2> m_webview;
    wil::com_ptr<ICoreWebView2Controller> m_webviewController;
    wil::com_ptr<ICoreWebView2Environment> m_environment;
    wil::com_ptr<ICoreWebView2> m_webview;

    std::string m_pendingUrl;
    bool m_webviewReady;
    int m_width, m_height;
};

// #endif
