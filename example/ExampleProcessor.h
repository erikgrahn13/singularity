#pragma once

#include "../core/SingularityProcessor.h"

class ExampleProcessor : public SingularityProcessor
{
  public:
    ExampleProcessor();
    ~ExampleProcessor();
    void Process() override;
    void Prepare() override;
};
