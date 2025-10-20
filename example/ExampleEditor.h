#pragma once

#include "../core/SingularityEditor.h"

// Plugin window size configuration

class ExampleEditor : public SingularityEditor
{

  public:
    ExampleEditor();
    ExampleEditor(bool createWindow); // Constructor for VST3 mode
    void Initialize() override;
};
