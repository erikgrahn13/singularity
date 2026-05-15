#include "ISingularityAudio.h"

// static std::unordered_map<int, Parameter> params;
class ParameterContainer : public IParameterProvider {
public:
    std::function<void(int, double)> onParameterChanged;
    double getParameter(int id) override {
        auto it = params.find(id);
        if (it != params.end()) return it->second.value;
        return std::numeric_limits<double>::quiet_NaN();
    }
    void setParameter(int id, double value) override {
        auto it = params.find(id);
        if (it != params.end()) 
        {
            it->second.value = value;
            if (onParameterChanged) onParameterChanged(id, value);
        }
    }
    std::unordered_map<int, Parameter> params;
} parameterContainer;

IParameterProvider& getParameterContainer() { return parameterContainer; }
void setOnParameterChanged(std::function<void(int, double)> cb) { parameterContainer.onParameterChanged = std::move(cb); }

void createParameter(int id, const char* name, ParamType type,
                     double defaultValue, double minValue, double maxValue)
{
    parameterContainer.params[id] = { name, type, defaultValue, minValue, maxValue, defaultValue };
}

std::map<int, double> getDefaultParams()
{
    std::map<int, double> result;
    for (auto& [id, param] : parameterContainer.params)
        result[id] = param.defaultValue;
    return result;
}