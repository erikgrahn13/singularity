#pragma once

// #include "../IParameterBackend.h"

#include <functional>
#include <unordered_map>
#include <vector>

#include "SingularityPlugin.h"

class APPController  {
public:
    using ParameterCallback = std::function<void(int, double)>;

    APPController(
        std::span<const Parameter> parameters,
        ParameterCallback callback);

    // std::span<const Parameter> parameterDefinitions() const override;
    double getParameter(unsigned int id);
    void setParameter(unsigned int id, double value);

private:
    std::vector<Parameter> parameters_;
    std::unordered_map<int, double> values_;
    ParameterCallback callback_;
};
