#pragma once

class IParameterStore {
public:
    virtual ~IParameterStore() = default;
    virtual void setParameter(int id, double value) = 0;
    virtual double getParameter(int id) = 0;
};
