#pragma once

#include "IRenderer2.h"
#include "IParameterProvider.h"
#include <functional>
#include <memory>
#include <vector>
#include <string>

class IJSEngine {
    public:
    using LogCallback = std::function<void(const std::string&)>;
    virtual ~IJSEngine() = default;
    static std::unique_ptr<IJSEngine> createJSEngine(IParameterProvider &parameterStore);
    virtual void load(const std::string& entryFile, IRenderer* renderer) = 0;
    virtual void setLogger(LogCallback cb) = 0;
    
    // virtual void hotReload() = 0;
    // virtual void setupJS() = 0;
    // virtual void loadScript(const std::string& path) = 0;
    // virtual void renderUI() = 0;
    // Events
    virtual void onMouseDown(void* component, float x, float y) = 0;
    virtual void onMouseUp(void* component, float x, float y) = 0;
    virtual void onMouseDrag(void* component, float x, float y) = 0;
    virtual void onMouseEnter(void* component) = 0;
    virtual void onMouseExit(void* component) = 0;
    // // Callbacks
    // virtual void setOnOpenSettings(std::function<void()> cb) = 0;
    // virtual void setOnSetBloom(std::function<void(float, float)> cb) {}
    // // Generic named string lists (used by native modules)
    // virtual void setStringList(const std::string& key, std::vector<std::string> values) = 0;
    protected:
    LogCallback logger_;

};
