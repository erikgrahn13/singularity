#pragma once

#include <memory>
#include <string>

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

    static std::unique_ptr<ISingularityGUI> createView(void *windowHandle = nullptr);
};
