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

using ParameterDefinition = std::variant<
    ContinuousParameter
>;

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

    static Parameter continuous(ContinuousOptions options);

    unsigned int id;
    std::string name;

    ParameterDefinition definition;
};

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

class ParameterValues
{
public:
    ParameterValues(std::span<const unsigned int> ids, std::span<const std::vector<double>> buffers)
    : ids_(ids), buffers_(buffers) {}
    
    double get(unsigned int id, int sampleOffset) const
    {
        for(int i = 0; i < ids_.size(); ++i)
        {
            if(ids_[i] == id)
                return buffers_[i][sampleOffset];
        }

        return 0.0;
    }
    void set(unsigned int id, double value){}

 private:
    std::span<const unsigned int> ids_;
    std::span<const std::vector<double>> buffers_;
};

struct MidiEvent{};


