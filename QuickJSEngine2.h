#pragma once

#include "IJSEngine.h"
#include "IParameterProvider.h"
#include "quickjs-libc.h"
#include <vector>

class QuickJSEngine : public IJSEngine {
    public:
    QuickJSEngine(IParameterProvider &parameterStore);
    void load(const std::string& entryFile, IRenderer* renderer) override;

    void installConsole();
    void log(const std::string& msg);
    

    void setLogger(LogCallback cb) override {
        logger_ = std::move(cb);
    }

    // Events
    JSValue on(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
    void registerMouseDownHandler(void* component, JSContext* ctx, JSValue fn);
    void registerMouseUpHandler(void* component, JSContext* ctx, JSValue fn);
    void registerMouseDragHandler(void* component, JSContext* ctx, JSValue fn);
    void registerMouseEnterHandler(void* component, JSContext* ctx, JSValue fn);
    void registerMouseExitHandler(void* component, JSContext* ctx, JSValue fn);
    void registerMouseWheelHandler(void* component, JSContext* ctx, JSValue fn);
    void onMouseDown(void* component, float x, float y) override;
    void onMouseUp(void* component, float x, float y) override;
    void onMouseDrag(void* component, float x, float y) override;
    void onMouseEnter(void* component) override;
    void onMouseExit(void* component) override;
    void onMouseWheel(void* component, float deltaX, float deltaY) override;

    JSValue getParameter(JSContext *ctx, JSValue this_val, int argc, JSValue *argv);
    JSValue setParameter(JSContext *ctx, JSValue this_val, int argc, JSValue *argv);



    JSValue appFn_ = JS_UNDEFINED;
    std::vector<JSValue> drawCallbacks_;

    private:
    void dispatchEvent(const std::string& type, float x, float y);
    JSRuntime* rt_{nullptr};
    JSContext* ctx_{nullptr};
    IRenderer* renderer_{nullptr};
    std::unordered_map<void*, JSValue> mouseDownHandlers_;
    std::unordered_map<void*, JSValue> mouseUpHandlers_;
    std::unordered_map<void*, JSValue> mouseDragHandlers_;
    std::unordered_map<void*, JSValue> mouseEnterHandlers_;
    std::unordered_map<void*, JSValue> mouseExitHandlers_;
    std::unordered_map<void*, JSValue> mouseWheelHandlers_;

    IParameterProvider& parameterStore_;

};