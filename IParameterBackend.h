#pragma once

#include <span>
#include <string>
#include <cassert>
#include <array>
#include <utility>
#include <vector>


enum class ParamType { Float, Bool, Stepped, Choice };

struct ParameterChange
{
    int id;
    double value;
};

struct Parameter
{
    unsigned int id = 0;
    std::string name;
    std::string shortName;
    std::string units;
    ParamType type = ParamType::Float;

    double minValue = 0.0;
    double maxValue = 1.0;
    double defaultValue = 0.0;

    int steps = 0;
    std::vector<std::string> choices;

    bool automatable = true;
    bool readOnly = false;
    bool wrapAround = false;

    int32_t groupId = 0;
};

struct ParamList
{
    // Values are plain framework values, not host-normalized transport values.
    using ParamValue = std::pair<unsigned int, double>;
    std::span<ParamValue> data;
    // Optional per-sample values, indexed in parallel with data. Adapters that
    // do not provide automation buffers leave this span empty.
    std::span<const std::span<const double>> sampleData {};

    double getValueAtSample (unsigned int id, int sampleOffset) const
    {
        for (std::size_t index = 0; index < data.size(); ++index)
        {
            if (data[index].first != id)
                continue;

            if (index < sampleData.size() && !sampleData[index].empty())
            {
                assert(sampleOffset >= 0 &&
                    static_cast<std::size_t>(sampleOffset) < sampleData[index].size());
                if (sampleOffset >= 0 &&
                    static_cast<std::size_t>(sampleOffset) < sampleData[index].size())
                    return sampleData[index][static_cast<std::size_t>(sampleOffset)];
            }

            return data[index].second;
        }

        assert(false && "requested parameter ID is not present in ParamList");
        return 0.0;
    }

    void set (unsigned int id, double value)
    {
        for (auto& [pid, currentValue] : data)
            if (pid == id)
            {
                currentValue = value;
                return;
            }
    }
};

struct MidiEvent
{
    enum class Type : uint8_t { NoteOn, NoteOff };

    Type    type;
    int16_t pitch;    ///< [0, 127]
    float   velocity; ///< [0.0, 1.0]
};

class IParameterBackend
{
    public:
    virtual ~IParameterBackend() = default;

    virtual std::span<const Parameter> parameterDefinitions() const = 0;
    virtual double getParameter(int id) const = 0;
    virtual void setParameter(int id, double value) = 0;
};