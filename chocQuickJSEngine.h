#pragma once

#include "IJSEngine.h"
#include "choc/javascript/choc_javascript_QuickJS.h"


class ChocQuickJSEngine : public IJSEngine {
public:
    ChocQuickJSEngine();
    void hotReload() override;
    void bindRenderer(IRenderer *renderer) override;
    void onMouseDown(float x, float y) override;

private:
    choc::javascript::Context ctx;
    IRenderer* currentRenderer = nullptr;
    std::string tmp;
};