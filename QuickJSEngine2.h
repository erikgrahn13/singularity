#pragma once

#include "IJSEngine.h"
#include "quickjs-libc.h"
#include <vector>

class QuickJSEngine : public IJSEngine {
    public:
    void load(const std::string& entryFile, IRenderer* renderer) override;

    JSValue appFn_ = JS_UNDEFINED;
    std::vector<JSValue> drawCallbacks_;
    private:
    

    JSRuntime* rt_{nullptr};
    JSContext* ctx_{nullptr};
    IRenderer* renderer_{nullptr};
};