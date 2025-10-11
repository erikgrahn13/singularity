#include "vst3editor.h"

using namespace Steinberg;

namespace MyCompanyName
{

//------------------------------------------------------------------------
SingularityVST3Editor::SingularityVST3Editor(EditController *controller, const std::string &url)
    : EditorView(controller, nullptr), initialUrl(url), defaultWidth(800), defaultHeight(600)
{
    // Create the WebView instance
    webview = ISingularityGUI::createView();
}

//------------------------------------------------------------------------
SingularityVST3Editor::~SingularityVST3Editor()
{
    if (webview)
    {
        webview->close();
    }
}

//------------------------------------------------------------------------
tresult PLUGIN_API SingularityVST3Editor::isPlatformTypeSupported(FIDString type)
{
#ifdef _WIN32
    if (strcmp(type, kPlatformTypeHWND) == 0)
        return kResultOk;
#elif __APPLE__
    if (strcmp(type, kPlatformTypeNSView) == 0)
        return kResultOk;
#elif __linux__
    if (strcmp(type, kPlatformTypeX11EmbedWindowID) == 0)
        return kResultOk;
#endif
    return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API SingularityVST3Editor::attached(void *parent, FIDString type)
{
    if (webview)
    {
        // Create WebView as child of host window
        webview->createAsChild(parent, defaultWidth, defaultHeight);
        webview->navigate(initialUrl);
        return kResultOk;
    }
    return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API SingularityVST3Editor::removed()
{
    if (webview)
    {
        webview->close();
    }
    return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API SingularityVST3Editor::getSize(ViewRect *size)
{
    if (size)
    {
        size->left = 0;
        size->top = 0;
        size->right = defaultWidth;
        size->bottom = defaultHeight;
        return kResultOk;
    }
    return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API SingularityVST3Editor::onSize(ViewRect *newSize)
{
    if (newSize && webview)
    {
        int width = newSize->right - newSize->left;
        int height = newSize->bottom - newSize->top;
        webview->resize(width, height);
        return kResultOk;
    }
    return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API SingularityVST3Editor::canResize()
{
    return kResultTrue;
}

//------------------------------------------------------------------------
tresult PLUGIN_API SingularityVST3Editor::checkSizeConstraint(ViewRect *rect)
{
    if (rect)
    {
        // Enforce minimum size
        int width = rect->right - rect->left;
        int height = rect->bottom - rect->top;

        if (width < 400)
            rect->right = rect->left + 400;
        if (height < 300)
            rect->bottom = rect->top + 300;

        return kResultOk;
    }
    return kResultFalse;
}

//------------------------------------------------------------------------
void SingularityVST3Editor::navigate(const std::string &url)
{
    if (webview)
    {
        webview->navigate(url);
    }
}

} // namespace MyCompanyName