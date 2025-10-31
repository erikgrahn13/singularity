#pragma once

#include "../../singularity_Webview.h"
#include "SingularityView/SingularityView-swift.h"

class WebViewMacos : public ISingularityGUI
{
  public:
    WebViewMacos(void *windowHandle = nullptr, std::function<void()> onReady = nullptr);
    ~WebViewMacos();

    void create(int width, int height, const std::string &title) override {};
    void createAsChild(void *parentWindow, int width, int height) override {};
    void navigate(const std::string &url) override;
    void run() override {};
    void close() override {};
    void resize(int width, int height) override {};
    void executeScript(const std::string &script) override {};
    void initialize() override;

  private:
    std::optional<SingularityView::WebViewMacos> swiftWebView;
    void *m_windowHandle;
};