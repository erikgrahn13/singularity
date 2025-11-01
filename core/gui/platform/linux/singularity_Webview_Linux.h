#pragma once

#include "../../singularity_Webview.h"
#include <gtk/gtk.h>
#include <memory>
#include <string>
#include <webkit/webkit.h>

class WebViewLinux : public ISingularityGUI
{
  public:
    WebViewLinux(void *windowHandle = nullptr, std::function<void()> onReady = nullptr);
    ~WebViewLinux();

    void create(int width, int height, const std::string &title) override;
    void createAsChild(void *parentWindow, int width, int height) {};
    void navigate(const std::string &url) override;
    void run() override;
    void close() override;
    void resize(int width, int height) override;
    void executeScript(const std::string &script);
    void initialize() override;

  private:
    GtkWidget *m_window;
    GtkWidget *m_webView;
};
