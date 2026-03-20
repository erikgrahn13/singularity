#include "QuickJSEngine.h"
#include <print>

std::unique_ptr<IJSEngine> createJSEngine()
{
    return std::make_unique<QuickJSEngine>();
}



QuickJSEngine::QuickJSEngine()
{
    rt = JS_NewRuntime();
    JS_SetModuleLoaderFunc2(rt, nullptr, js_module_loader, js_module_check_attributes, nullptr);
    ctx =  JS_NewContext(rt);

    
    size_t buf_len;
    std::string path = JS_SCRIPTS_DIR"/hello.js";
    auto tmp = js_load_file(ctx, &buf_len, path.c_str());
    std::println("QuickJSEngine init {}/hello.js",JS_SCRIPTS_DIR);

    JSValue hello = JS_NewCFunction(ctx, js_hello, "hello", 1);

    JSValue global_obj = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global_obj, "hello", hello);
    JS_FreeValue(ctx, global_obj);

    JSValue result = JS_Eval(ctx, (const char*)tmp, buf_len, path.c_str(), JS_EVAL_TYPE_MODULE);

}

QuickJSEngine::~QuickJSEngine()
{
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
}

JSValue QuickJSEngine::js_hello(JSContext *ctx, JSValue this_val, int argc, JSValue* argv)
{
    // for(int i = 0; i < argc; ++i)
    // {
    //     std::cout << ""
    // }
    std::println("Hello from Javascript");
}

