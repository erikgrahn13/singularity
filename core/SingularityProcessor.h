#pragma once

#include <memory>

class SingularityProcessor
{
  public:
    virtual void Process() = 0;
    virtual void Prepare() = 0;
};

std::unique_ptr<SingularityProcessor> createProcessorInstance();
