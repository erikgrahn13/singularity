#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

class ISingularityGUI
{
  public:
    ISingularityGUI();
    virtual ~ISingularityGUI();

    // Core WebView functionality (standalone)
    virtual void create(int width, int height, const std::string &title) = 0;
    virtual void createAsChild(void *parentWindow, int width, int height) = 0;
    virtual void navigate(const std::string &url) = 0;
    virtual void run() = 0;
    virtual void close() = 0;
    virtual void resize(int width, int height) = 0;
    virtual void executeScript(const std::string &script) = 0;
    virtual void initialize() = 0;

    void setViewReady(std::function<void()> callback)
    {
        m_viewReadyCallback = callback;
    }

    void OnWebViewReady()
    {
        if (m_viewReadyCallback)
        {
            m_viewReadyCallback();
        }
    }

    static std::unique_ptr<ISingularityGUI> createView(void *windowHandle, std::function<void()> onReady);
    std::vector<std::string> m_pendingScripts;

  protected:
    std::function<void()> m_viewReadyCallback;
};
