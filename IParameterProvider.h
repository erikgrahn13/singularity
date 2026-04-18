#pragma once
#include <string>
#include <unordered_map>

struct ParameterChange
{
    int id;
    double value;
};

enum class ParamType { Float, Bool, Stepped };

struct Parameter {
    std::string name;
    ParamType type = ParamType::Float;
    double value = 0.0;
    double minValue = 0.0;
    double maxValue = 1.0;
    double defaultValue = 0.0;
    int steps = 0; // Only used for Stepped type
};

class IParameterProvider {
public:
    virtual double getParameter(int id) const = 0;
    virtual void setParameter(int id, double value) = 0;
    virtual ~IParameterProvider() = default;
};

class IParameterChanges {
public:
    virtual int getCount() const = 0;
    virtual ParameterChange get(int index) const = 0;
    virtual ~IParameterChanges() = default;
};