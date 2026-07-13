#pragma once
#include <array>
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
    bool isBypass = false;
    bool isList = false;
    bool isProgramChange = false;

    int32_t unitId = 0;
};

struct ParameterGroup {
    int32_t id = 0;
    int32_t parentId = 0;
    std::string name;
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