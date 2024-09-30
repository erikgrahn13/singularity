#pragma once

#include "../core/Editor.h"

class ExampleEditor : public Editor
{

  public:
    ExampleEditor(int width, int height);
    void draw(SkCanvas *canvas) override;
};

std::unique_ptr<Editor> createEditorInstance()
{
    return std::make_unique<ExampleEditor>(800, 600);
}