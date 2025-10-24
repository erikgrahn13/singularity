#include "vst3editor.h"
#include "platform/VST3Window.h"

using namespace Steinberg;

namespace MyCompanyName
{

//------------------------------------------------------------------------
SingularityVST3Editor::SingularityVST3Editor(EditController *controller, SingularityController *sharedController)
    : EditorView(controller, nullptr), audioController(sharedController)
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
    if (audioController)
    {
        // Create WebView as child of host window using the ExampleEditor's webview
        void *childWindow = VST3Window::createPlatformWindow(parent);
        auto view = ISingularityGUI::createView(childWindow);
        Singularity::Internal::setControllerView(audioController, std::move(view));
        return kResultOk;
    }
    return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API SingularityVST3Editor::removed()
{
    if (audioController && audioController->getWebView())
    {
        audioController->getWebView()->close();
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
    if (newSize && audioController && audioController->getWebView())
    {
        int width = newSize->right - newSize->left;
        int height = newSize->bottom - newSize->top;
        audioController->getWebView()->resize(width, height);
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
    if (audioController && audioController->getWebView())
    {
        audioController->getWebView()->navigate(url);
    }
}

} // namespace MyCompanyName