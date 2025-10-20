#include "ExampleEditor.h"

ExampleEditor::ExampleEditor()
{
    // setSize(PLUGIN_WIDTH, PLUGIN_HEIGHT);
    int hej;
    hej = 3;
}

ExampleEditor::ExampleEditor(bool createWindow) : SingularityEditor(createWindow)
{
    // Pass createWindow to base class
    // setSize(PLUGIN_WIDTH, PLUGIN_HEIGHT);
    int hej;
    hej = 3;
}

void ExampleEditor::Initialize()
{
    // Here you should register you parameters
}

std::unique_ptr<SingularityEditor> createEditorInstance()
{
    return std::make_unique<ExampleEditor>();
}

// Factory function for VST3 mode (no window creation)
std::unique_ptr<SingularityEditor> createEditorInstanceForVST3()
{
    return std::make_unique<ExampleEditor>(false); // false = don't create window
}