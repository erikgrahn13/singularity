#pragma once

// #ifdef _WIN32
#include "../../singularity_Webview.h"
#include <WebView2.h>
#include <string>
#include <windows.h>
#include <wrl.h>

using namespace Microsoft::WRL;

class WebViewWindows : public ISingularityGUI
{
  public:
    WebViewWindows();
    ~WebViewWindows();

    // ISingularityGUI interface (standalone + child window support)
    void create(int width, int height, const std::string &title) override;
    void createAsChild(void *parentWindow, int width, int height) override;
    void navigate(const std::string &url) override;
    void run() override;
    void close() override;
    void resize(int width, int height) override;

  private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void createWebView();
    void onWebViewCreated(ICoreWebView2Controller *controller);
    void setWebViewBounds();

    HWND m_hwnd;
    ComPtr<ICoreWebView2Environment> m_environment;
    ComPtr<ICoreWebView2Controller> m_controller;
    ComPtr<ICoreWebView2> m_webview;

    std::string m_pendingUrl;
    bool m_webviewReady;
    int m_width, m_height;
};

// #endif
