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
    virtual void draw() = 0;
    virtual void setLogger(LogCallback cb) = 0;
    
    // Mouse events — C++ sends raw window coordinates, JS owns hit testing and routing
    virtual void onMouseDown(float x, float y) = 0;
    virtual void onMouseUp(float x, float y) = 0;
    virtual void onMouseMove(float x, float y) = 0;
    virtual void onMouseWheel(float deltaX, float deltaY) = 0;
    protected:
    LogCallback logger_;

};
