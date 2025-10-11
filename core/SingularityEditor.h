#pragma once

#include "gui/singularity_Webview.h"
#include <memory>

class SingularityEditor
{
  public:
    SingularityEditor();
    virtual ~SingularityEditor();
    virtual void draw() = 0;
    void loadFont();

  protected:
    // virtual void initializeSkiaSurface() = 0;

    int width;
    int height;
    std::unique_ptr<ISingularityGUI> webview;
};

std::unique_ptr<SingularityEditor> createEditorInstance();
