#include "IJSEngine.h"
#include <quickjs-libc.h>

class QuickJSEngine : public IJSEngine {
    public:
    QuickJSEngine();
    ~QuickJSEngine();

    private:
    JSRuntime *rt;
    JSContext *ctx;
};
