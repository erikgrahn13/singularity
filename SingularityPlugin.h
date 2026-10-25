#pragma once
#include <span>
#include "SingularityParameter.h"
#include <cstdint>
#include <optional>

using ActionValue = std::variant< std::monostate, bool, std::int64_t, double, std::string>;
using ActionResult = std::optional<std::pair<std::string, ActionValue>>;

template<typename SampleType>
using AudioInputBuffer = std::span<const SampleType* const>;

template<typename SampleType>
using AudioOutputBuffer = std::span<SampleType* const>;

template<typename P>
concept SingularityFxPlugin =
    requires(P& plugin,
        AudioInputBuffer<float> fin, AudioOutputBuffer<float> fout,
        AudioInputBuffer<double> din, AudioOutputBuffer<double> dout,
        int n, ParameterValues params,
        double sampleRate, int maxBlockSize,
        std::string_view name, const ActionValue& value)
    {
        plugin.template process<float>  (fin,  fout, n, params);
        plugin.template process<double> (din,  dout, n, params);
        plugin.prepare(sampleRate, maxBlockSize);
        plugin.handleAction(name, value);
        { P::getParameters() };
        requires !P::isInstrument;
    };

template<typename P>
concept SingularityInstrumentPlugin =
    requires(P& plugin,
        AudioOutputBuffer<float>  fout,
        AudioOutputBuffer<double> dout,
        int n, std::span<const MidiEvent> events,
        ParameterValues params,
        double sampleRate, int maxBlockSize,
        std::string_view name, const ActionValue& value)
    {
        plugin.template process<float>  (fout, n, events, params);
        plugin.template process<double> (dout, n, events, params);
        plugin.prepare(sampleRate, maxBlockSize);
        plugin.handleAction(name, value);
        { P::getParameters() };
        requires P::isInstrument;
    };

class UiSender
{
public:
    template<typename Backend>
    void connect(Backend &backend)
    {
        instance_ = &backend;

        sendFloat_ = [](void* instance, AudioInputBuffer<float> data, int numSamples)
        {
            static_cast<Backend*>(instance)->sendUiData(data, numSamples);
        };

        sendDouble_ = [](void* instance, AudioInputBuffer<double> data, int numSamples)
        {
            static_cast<Backend*>(instance)->sendUiData(data, numSamples);
        };
    }

    void operator()(AudioInputBuffer<float> data, int numSamples) const
    {
        sendFloat_(instance_, data, numSamples);
    }

    void operator()(AudioInputBuffer<double> data, int numSamples) const
    {
        sendDouble_(instance_, data, numSamples);
    }

private:
    void* instance_{nullptr};

    void (*sendFloat_)(void*, AudioInputBuffer<float>, int) = nullptr;
    void (*sendDouble_)(void*, AudioInputBuffer<double>, int) = nullptr;
};

template<typename P>
concept SingularityPlugin = SingularityFxPlugin<P> || SingularityInstrumentPlugin<P>;
