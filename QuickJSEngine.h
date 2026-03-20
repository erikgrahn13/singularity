#include "IJSEngine.h"
#include <quickjs-libc.h>

class QuickJSEngine : public IJSEngine {
    public:
    QuickJSEngine();
    ~QuickJSEngine();

    static JSValue js_hello(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);

    private:
    JSRuntime *rt;
    JSContext *ctx;
};
