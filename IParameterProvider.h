#pragma once
#include <array>
#include <cassert>
#include <cstdint>
#include <string>
#include <span>
#include <utility>
#include <unordered_map>
#include <vector>
#include <string_view>

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

class IParameterProvider {
public:
    virtual double getParameter(int id) = 0;
    virtual void setParameter(int id, double value) = 0;
    virtual double getSampleRate() const { return 0.0; }
    virtual std::string getPluginState() const { return {}; }
    virtual void sendMessage(std::string_view, std::string_view) {}
    virtual ~IParameterProvider() = default;
};
