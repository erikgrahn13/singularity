#include "vst3editor.h"

using namespace Steinberg;

// Forward declaration of createEditorInstanceForVST3 (defined in ExampleEditor.cpp)
extern std::unique_ptr<SingularityEditor> createEditorInstanceForVST3();

namespace MyCompanyName
{

//------------------------------------------------------------------------
SingularityVST3Editor::SingularityVST3Editor(EditController *controller, SingularityEditor *sharedEditor)
    : EditorView(controller, nullptr), audioEditor(sharedEditor)
{
    // Use the shared editor instance from the controller
    // No need to create a new instance here
}

//------------------------------------------------------------------------
SingularityVST3Editor::~SingularityVST3Editor()
{
    // Don't delete audioEditor - it's owned by the controller
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
    if (audioEditor && audioEditor->getWebView())
    {
        // Create WebView as child of host window using the ExampleEditor's webview
        audioEditor->getWebView()->createAsChild(parent, PLUGIN_WIDTH, PLUGIN_HEIGHT);
        audioEditor->getWebView()->navigate("http://localhost:5173/");
        return kResultOk;
    }
    return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API SingularityVST3Editor::removed()
{
    if (audioEditor && audioEditor->getWebView())
    {
        audioEditor->getWebView()->close();
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
        size->right = PLUGIN_WIDTH;
        size->bottom = PLUGIN_HEIGHT;
        return kResultOk;
    }
    return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API SingularityVST3Editor::onSize(ViewRect *newSize)
{
    if (newSize && audioEditor && audioEditor->getWebView())
    {
        int width = newSize->right - newSize->left;
        int height = newSize->bottom - newSize->top;
        audioEditor->getWebView()->resize(width, height);
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
    // if (rect)
    // {
    //     // Enforce minimum size
    //     int width = rect->right - rect->left;
    //     int height = rect->bottom - rect->top;

    //     if (width < 400)
    //         rect->right = rect->left + 400;
    //     if (height < 300)
    //         rect->bottom = rect->top + 300;

    //     return kResultOk;
    // }
    // return kResultFalse;
    return kResultTrue;
}

//------------------------------------------------------------------------
void SingularityVST3Editor::navigate(const std::string &url)
{
    if (audioEditor && audioEditor->getWebView())
    {
        audioEditor->getWebView()->navigate(url);
    }
}

} // namespace MyCompanyName