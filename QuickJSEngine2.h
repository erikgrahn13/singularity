#pragma once

#include "IJSEngine.h"
#include "quickjs-libc.h"
#include <vector>

class QuickJSEngine : public IJSEngine {
    public:
    void load(const std::string& entryFile, IRenderer* renderer) override;

    // Events
    JSValue on(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
    void registerMouseDownHandler(void* component, JSContext* ctx, JSValue fn);
    void registerMouseUpHandler(void* component, JSContext* ctx, JSValue fn);
    void registerMouseDragHandler(void* component, JSContext* ctx, JSValue fn);
    void onMouseDown(void* component, float x, float y) override;
    void onMouseUp(void* component, float x, float y) override;
    void onMouseDrag(void* component, float x, float y) override;


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
};