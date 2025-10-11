#include "ExampleEditor.h"

ExampleEditor::ExampleEditor()
{
    // setSize(800, 600);
}

void ExampleEditor::draw()
{
}

std::unique_ptr<SingularityEditor> createEditorInstance()
{
    return std::make_unique<ExampleEditor>();
}