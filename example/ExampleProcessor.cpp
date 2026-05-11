#include "ExampleProcessor.h"

ExampleProcessor::ExampleProcessor()
{
}

ExampleProcessor::~ExampleProcessor()
{
}

void ExampleProcessor::Process()
{
}

void ExampleProcessor::Prepare()
{
}

std::unique_ptr<SingularityProcessor> createProcessorInstance()
{
    return std::make_unique<ExampleProcessor>();
}
