#pragma once
#include <span>
#include "SingularityParameter.h"
#include <cstdint>
#include <optional>

using ActionValue = std::variant< std::monostate, bool, std::int64_t, double, std::string>;
using ActionResult = std::optional<std::pair<std::string, ActionValue>>;

template<typename P>
concept SingularityFxPlugin =
    requires(P& plugin,
        std::span<const float* const>  fin,  std::span<float* const>  fout,
        std::span<const double* const> din,  std::span<double* const> dout,
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
        std::span<float* const>  fout,
        std::span<double* const> dout,
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

template<typename P>
concept SingularityPlugin = SingularityFxPlugin<P> || SingularityInstrumentPlugin<P>;
