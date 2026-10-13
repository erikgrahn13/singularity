#pragma once

#include <string>
#include <variant>
#include <span>
#include <vector>
// #include <unordered_map>

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
    };

    struct BooleanOptions
    {
        unsigned int id;
        std::string name;

        bool defaultValue;
    };

    struct SteppedOptions
    {
        unsigned int id;
        std::string name;

        int minValue;
        int maxValue;
        int defaultValue;
    };

    struct ChoiceOptions
    {
        unsigned int id;
        std::string name;

        std::vector<std::string> choices;
        unsigned int defaultIndex;
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
        }
    };
}

inline Parameter Parameter::boolean(BooleanOptions options)
{
    return {
        .id = options.id,
        .name = std::move(options.name),
        .definition = BooleanParameter{
            .defaultValue = options.defaultValue
        }
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
        }
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
        }
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

struct MidiEvent{};


