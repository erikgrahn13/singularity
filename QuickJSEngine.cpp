#include "QuickJSEngine.h"

std::unique_ptr<IJSEngine> createJSEngine()
{
    return std::make_unique<QuickJSEngine>();
}