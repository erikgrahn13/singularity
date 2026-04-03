#pragma once
#include <string>

class IParameterProvider {
public:
    virtual double getParameter(int id) const = 0;
    virtual void setParameter(int id, double value) = 0;
    virtual ~IParameterProvider() = default;
};