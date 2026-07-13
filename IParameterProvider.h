#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <span>
#include <utility>
#include <unordered_map>
#include <vector>

struct ParameterChange
{
    int id;
    double value;
};

enum class ParamType { Float, Bool, Stepped, Choice };

struct Parameter {
    unsigned int id = 0;
    std::string name;
    std::string shortName;
    std::string units;
    ParamType type = ParamType::Float;

    // Framework-facing parameter values are plain values in the plugin author's
    // declared units. VST3-normalized 0.0-1.0 values must be converted at the
    // VST3 adapter boundary before reaching shared DSP/UI APIs.
    double minValue = 0.0;
    double maxValue = 1.0;
    double defaultValue = 0.0;

    int steps = 0; // Only used for Stepped type
    std::vector<std::string> choices; // Used for Choice parameters

    bool automatable = true;
    bool readOnly = false;
    bool wrapAround = false;

    // Generic parameter grouping. Adapters can map this to their own grouping
    // system (for example VST3 UnitID) without exposing format-specific names.
    int32_t groupId = 0;

    double toNormalized(double plainValue) const
    {
        if (type == ParamType::Bool)
            return plainValue >= 0.5 ? 1.0 : 0.0;

        if (type == ParamType::Choice && !choices.empty())
        {
            const auto maxIndex = static_cast<double>(choices.size() - 1);
            if (maxIndex <= 0.0)
                return 0.0;
            return std::clamp(std::round(plainValue) / maxIndex, 0.0, 1.0);
        }

        if (maxValue == minValue)
            return 0.0;

        return std::clamp((plainValue - minValue) / (maxValue - minValue), 0.0, 1.0);
    }

    double toPlain(double normalizedValue) const
    {
        const auto clamped = std::clamp(normalizedValue, 0.0, 1.0);

        if (type == ParamType::Bool)
            return clamped >= 0.5 ? 1.0 : 0.0;

        if (type == ParamType::Choice && !choices.empty())
            return std::round(clamped * static_cast<double>(choices.size() - 1));

        const auto plain = minValue + clamped * (maxValue - minValue);
        if (type == ParamType::Stepped)
            return std::round(plain);

        return plain;
    }
};


struct ParamList
{
    // Values are plain framework values, not host-normalized transport values.
    using ParamValue = std::pair<unsigned int, double>;
    std::span<const ParamValue> data;

    double get (unsigned int id, double fallback = 0.0) const
    {
        for (auto& [pid, val] : data)
            if (pid == id) return val;
        return fallback;
    }
};

struct MidiEvent
{
    enum class Type : uint8_t { NoteOn, NoteOff };

    Type    type;
    int16_t pitch;    ///< [0, 127]
    float   velocity; ///< [0.0, 1.0]
};

class IParameterProvider {
public:
    virtual double getParameter(int id) = 0;
    virtual void setParameter(int id, double value) = 0;
    virtual ~IParameterProvider() = default;
};