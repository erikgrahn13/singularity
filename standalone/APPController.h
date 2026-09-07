#pragma once

#include "../IParameterBackend.h"

#include <functional>
#include <unordered_map>
#include <vector>

class APPController : public IParameterBackend {
public:
    using ParameterCallback = std::function<void(int, double)>;

    APPController(
        std::span<const Parameter> parameters,
        ParameterCallback callback);

    std::span<const Parameter> parameterDefinitions() const override;
    double getParameter(int id) override;
    void setParameter(int id, double value) override;

private:
    std::vector<Parameter> parameters_;
    std::unordered_map<int, double> values_;
    ParameterCallback callback_;
};
