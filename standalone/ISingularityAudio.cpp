#include "ISingularityAudio.h"
#include <map>

class ParameterContainer : public IParameterProvider {
public:
    std::function<void(int, double)> onParameterChanged;
    double getParameter(int id) override {
        auto it = values.find(id);
        if (it != values.end()) return it->second;
        return std::numeric_limits<double>::quiet_NaN();
    }
    void setParameter(int id, double value) override {
        auto it = values.find(id);
        if (it != values.end())
        {
            it->second = value;
            if (onParameterChanged) onParameterChanged(id, value);
        }
    }
    std::map<int, double> values;
} parameterContainer;

IParameterProvider& getParameterContainer() { return parameterContainer; }
void setOnParameterChanged(std::function<void(int, double)> cb) { parameterContainer.onParameterChanged = std::move(cb); }

void populateParameterContainer(std::span<const Parameter> params)
{
    for (auto& p : params)
        parameterContainer.values[p.id] = p.defaultValue;
}