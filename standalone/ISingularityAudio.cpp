#include "ISingularityAudio.h"
#include <atomic>
#include <map>

class ParameterContainer : public IParameterProvider {
public:
    std::function<void(int, double)> onParameterChanged;
    double getParameter(int id) override {
        auto it = values.find(id);
        if (it != values.end()) return it->second.load(std::memory_order_relaxed);
        return std::numeric_limits<double>::quiet_NaN();
    }
    void setParameter(int id, double value) override {
        auto it = values.find(id);
        if (it != values.end())
        {
            it->second.store(value, std::memory_order_relaxed);
            if (onParameterChanged) onParameterChanged(id, value);
        }
    }
    void setOutputParameter(int id, double value) {
        auto it = values.find(id);
        if (it != values.end())
            it->second.store(value, std::memory_order_relaxed);
    }
    std::map<int, std::atomic<double>> values;
} parameterContainer;

IParameterProvider& getParameterContainer() { return parameterContainer; }
void setOnParameterChanged(std::function<void(int, double)> cb) { parameterContainer.onParameterChanged = std::move(cb); }
void setOutputParameter(int id, double value) { parameterContainer.setOutputParameter(id, value); }

void populateParameterContainer(std::span<const Parameter> params)
{
    for (auto& p : params)
        parameterContainer.values[p.id].store(p.defaultValue, std::memory_order_relaxed);
}
