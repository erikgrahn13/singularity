#pragma once
#include <memory>

class SingularityPlugin {

};

std::unique_ptr<SingularityPlugin> createPlugin();  