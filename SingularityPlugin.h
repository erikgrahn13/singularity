#pragma once
#include <span>
#include "IParameterProvider.h"

template<typename P>
concept SingularityPlugin =
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
    };
