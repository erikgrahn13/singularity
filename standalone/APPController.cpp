#include "APPController.h"

#include <limits>
#include <utility>

APPController::APPController(
    std::span<const Parameter> parameters,
    ParameterCallback callback)
    : parameters_(parameters.begin(), parameters.end()),
      callback_(std::move(callback))
{
    for (const auto& parameter : parameters_)
        values_.emplace(static_cast<int>(parameter.id), parameter.defaultValue);
}

std::span<const Parameter> APPController::parameterDefinitions() const
{
    return parameters_;
}

double APPController::getParameter(int id) const
{
    const auto parameter = values_.find(id);
    if (parameter == values_.end())
        return std::numeric_limits<double>::quiet_NaN();

    return parameter->second;
}

void APPController::setParameter(int id, double value)
{
    const auto parameter = values_.find(id);
    if (parameter == values_.end() || parameter->second == value)
        return;

    parameter->second = value;
    if (callback_)
        callback_(id, value);
}
