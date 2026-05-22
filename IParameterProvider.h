#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <span>
#include <utility>
#include <unordered_map>

struct ParameterChange
{
    int id;
    double value;
};

enum class ParamType { Float, Bool, Stepped };

struct Parameter {
    unsigned int id = 0;
    std::string name;
    ParamType type = ParamType::Float;
    double minValue = 0.0;
    double maxValue = 1.0;
    double defaultValue = 0.0;
    int steps = 0; // Only used for Stepped type
};

struct ParamList
{
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