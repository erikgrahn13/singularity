#pragma once
#include <span>
#include "IParameterProvider.h"

template<typename P>
concept SingularityFxPlugin =
    requires(P& plugin,
        std::span<const float* const>  fin,  std::span<float* const>  fout,
        std::span<const double* const> din,  std::span<double* const> dout,
        int n, ParamList params,
        double sampleRate, int maxBlockSize)
    {
        plugin.template process<float>  (fin,  fout,  n, params);
        plugin.template process<double> (din,  dout,  n, params);
        plugin.prepare(sampleRate, maxBlockSize);
        { P::getParameters() };
        requires !P::isInstrument;
    };

template<typename P>
concept SingularityInstrumentPlugin =
    requires(P& plugin,
        std::span<float* const>  fout,
        std::span<double* const> dout,
        int n, std::span<const MidiEvent> events, ParamList params,
        double sampleRate, int maxBlockSize)
    {
        plugin.template process<float>  (fout, n, events, params);
        plugin.template process<double> (dout, n, events, params);
        plugin.prepare(sampleRate, maxBlockSize);
        { P::getParameters() };
        requires P::isInstrument;
    };

template<typename P>
concept SingularityPlugin = SingularityFxPlugin<P> || SingularityInstrumentPlugin<P>;
