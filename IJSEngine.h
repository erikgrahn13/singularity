#pragma once

#include "IRenderer.h"
#include <functional>
#include <memory>
#include <vector>
#include <string>

class IJSEngine {
    public:
    virtual void hotReload() = 0;
    virtual void setupJS() = 0;
    virtual void loadScript(const std::string& path) = 0;
    virtual void renderUI() = 0;
    // Events
    virtual void onMouseDown(float x, float y) = 0;
    virtual void onMouseUp(float x, float y) = 0;
    virtual void onMouseMove(float x, float y) = 0;
    // Callbacks
    virtual void setOnOpenSettings(std::function<void()> cb) = 0;
    // Generic named string lists (used by native modules)
    virtual void setStringList(const std::string& key, std::vector<std::string> values) = 0;
};
