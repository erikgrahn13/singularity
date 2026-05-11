#include "SingularityView/SingularityView-swift.h"
#include "VST3Window.h"

void *VST3Window::createPlatformWindow(void *parent)
{
    return SingularityView::createChildNSView(parent, PLUGIN_WIDTH, PLUGIN_HEIGHT);
}
