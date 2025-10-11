#pragma once

#include <string>

class WebViewLinux
{
  public:
    WebViewLinux();
    ~WebViewLinux();

    void create(int width, int height, const std::string &title);
    void navigate(const std::string &url);
    void run();
    void close();

  private:
    void *m_window;  // GtkWidget*
    void *m_webView; // WebKitWebView*
};
