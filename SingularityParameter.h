#pragma once

#include <string>
#include <variant>
#include <span>
#include <vector>
#include <stdint.h>

struct ContinuousParameter
{
    double minValue;
    double maxValue;
    double defaultValue;
};

struct BooleanParameter
{
    bool defaultValue;
};

struct SteppedParameter
{
    int minValue;
    int maxValue;
    int defaultValue;
};

struct ChoiceParameter
{
    std::vector<std::string> choices;
    unsigned int defaultIndex;
};

using ParameterDefinition = std::variant<
    ContinuousParameter,
    BooleanParameter,
    SteppedParameter,
    ChoiceParameter
>;

enum class ParameterDirection
{
    input,
    output
};

struct Parameter
{
    struct ContinuousOptions
    {
        unsigned int id;
        std::string name;
        double minValue;
        double maxValue;
        double defaultValue;
        std::string unit;
        std::string group;
    };

    struct BooleanOptions
    {
        unsigned int id;
        std::string name;
        bool defaultValue;
        std::string unit;
        std::string group;
    };

    struct SteppedOptions
    {
        unsigned int id;
        std::string name;
        int minValue;
        int maxValue;
        int defaultValue;
        std::string unit;
        std::string group;
    };

    struct ChoiceOptions
    {
        unsigned int id;
        std::string name;
        std::vector<std::string> choices;
        unsigned int defaultIndex;
        std::string unit;
        std::string group;
    };

    static Parameter continuous(ContinuousOptions options);
    static Parameter boolean(BooleanOptions options);
    static Parameter stepped(SteppedOptions options);
    static Parameter choice(ChoiceOptions options);
    Parameter asOutput() &&;

    unsigned int id;
    std::string name;
    ParameterDirection direction{ ParameterDirection::input };
    ParameterDefinition definition;
    std::string unit;
    std::string group;
};

inline Parameter Parameter::asOutput() &&
{
    direction = ParameterDirection::output;
    return std::move(*this);
}

inline Parameter Parameter::continuous(ContinuousOptions options)
{
    return {
        .id = options.id,
        .name = std::move(options.name),
        .definition = ContinuousParameter{
            .minValue = options.minValue,
            .maxValue = options.maxValue,
            .defaultValue = options.defaultValue
        },
        .unit = options.unit,
        .group = options.group
    };
}

inline Parameter Parameter::boolean(BooleanOptions options)
{
    return {
        .id = options.id,
        .name = std::move(options.name),
        .definition = BooleanParameter{
            .defaultValue = options.defaultValue
        },
        .unit = options.unit,
        .group = options.group
    };
}

inline Parameter Parameter::stepped(SteppedOptions options)
{
    return {
        .id = options.id,
        .name = std::move(options.name),
        .definition = SteppedParameter {
            .minValue = options.minValue,
            .maxValue = options.maxValue,
            .defaultValue = options.defaultValue
        },
        .unit = options.unit,
        .group = options.group
    };
}

inline Parameter Parameter::choice(ChoiceOptions options)
{
    return {
        .id = options.id,
        .name = std::move(options.name),
        .definition = ChoiceParameter {
            .choices = std::move(options.choices),
            .defaultIndex = options.defaultIndex
        },
        .unit = options.unit,
        .group = options.group
    };
}

class ParameterValues
{
public:
    ParameterValues(std::span<const Parameter> parameters, std::span<std::vector<double>> buffers, std::span<bool> outputParameterWasSet)
    : parameters_(parameters), buffers_(buffers), outputParameterWasSet_(outputParameterWasSet) {}
    
    double get(unsigned int id, int sampleOffset) const
    {
        for(int i = 0; i < parameters_.size(); ++i)
        {
            if(parameters_[i].id == id && parameters_[i].direction == ParameterDirection::input)
                return buffers_[i][sampleOffset];
        }

        return 0.0;
    }
    void set(unsigned int id, double value)
    {
        for(int i = 0; i < parameters_.size(); ++i)
        {
            if(parameters_[i].id == id && parameters_[i].direction == ParameterDirection::output)
            {
                buffers_[i][0] = value;
                outputParameterWasSet_[i] = true;
                return;
            }
        }
    }

 private:
    std::span<const Parameter> parameters_;
    std::span<std::vector<double>> buffers_;
    std::span<bool> outputParameterWasSet_;
};

struct MidiEvent 
{
    struct NoteOn
    {
        int16_t channel{};
        int16_t pitch{};
        float velocity{};
        float tuning{};
        int32_t noteId{-1};
    };

    struct NoteOff
    {
        int16_t channel{};
        int16_t pitch{};
        float velocity{};
        float tuning{};
        int32_t noteId{-1};
    };

    int32_t sampleOffset;
    std::variant<NoteOn, NoteOff> data_;
};

using MidiEvents = std::span<const MidiEvent>;



