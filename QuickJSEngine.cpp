#include "QuickJSEngine.h"

std::unique_ptr<IJSEngine> createJSEngine()
{
    return std::make_unique<QuickJSEngine>();
}

QuickJSEngine::QuickJSEngine()
{
    rt = JS_NewRuntime();
    JS_SetModuleLoaderFunc2(rt, nullptr, js_module_loader, js_module_check_attributes, nullptr);
    ctx =  JS_NewContext(rt);
}

QuickJSEngine::~QuickJSEngine()
{
    JS_FreeRuntime(rt);
}