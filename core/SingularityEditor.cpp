#include "SingularityEditor.h"

SingularityEditor::SingularityEditor()
{
    webview = ISingularityGUI::createView();
    webview->create(800, 600, "Native WebView - React App");
    webview->navigate("http://localhost:5173/");
    webview->run();
}

SingularityEditor::~SingularityEditor()
{
}

void SingularityEditor::loadFont()
{
    // load the desired font here
    // sk_sp<SkTypeface> typeface = fontMgr->makeFromFile(fontPath);
}

// void SingularityEditor::draw()
// {
// }
