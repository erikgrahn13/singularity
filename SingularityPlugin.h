#pragma once
#include <span>
#include <map>
#include "IParameterProvider.h"

template<typename P>
concept SingularityPlugin =
    requires(P& plugin,
        std::span<const float* const>  fin,  std::span<float* const>  fout,
        std::span<const double* const> din,  std::span<double* const> dout,
        int n, const std::map<int, double>& params,
        double sampleRate, int maxBlockSize)
    {
        plugin.template process<float>  (fin,  fout,  n, params);
        plugin.template process<double> (din,  dout,  n, params);
        plugin.prepare(sampleRate, maxBlockSize);
        P::registerParameters();
    };

void createParameter(int id, const char* name, ParamType type,
                     double defaultValue, double minValue, double maxValue);
