#pragma once

#include "IJSEngine.h"
#include "choc/javascript/choc_javascript_QuickJS.h"


class ChocQuickJSEngine : public IJSEngine {
public:
    ChocQuickJSEngine();
    void hotReload() override;
    void bindRenderer(IRenderer *renderer) override;

private:
    choc::javascript::Context ctx;
    std::string tmp;
};