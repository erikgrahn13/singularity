#include "SingularityController.h"

SingularityController::SingularityController() : SingularityController(true)
{
    // Default constructor creates window (for standalone mode)
}

SingularityController::SingularityController(bool createWindow)
{
    // webview = ISingularityGUI::createView();

    if (createWindow)
    {
        // Standalone mode: create native window
        webview->create(PLUGIN_WIDTH, PLUGIN_HEIGHT, "Native WebView - React App");
        webview->navigate("http://localhost:5173/");
        webview->run();
    }
    // VST3 mode: just create webview instance, don't create window
    // The window will be created later via createAsChild()
}

SingularityController::~SingularityController()
{
}

// void SingularityEditor::setSize(int width, int height)
// {
//     webview->resize(width, height);
// }

// void SingularityEditor::loadFont()
// {
//     // load the desired font here
//     // sk_sp<SkTypeface> typeface = fontMgr->makeFromFile(fontPath);
// }

// void SingularityEditor::draw()
// {
// }
