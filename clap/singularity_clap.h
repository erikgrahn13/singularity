#pragma once

#include "clap/helpers/plugin.hh"

#include "SingularityPlugin.h"

template<SingularityPlugin PluginType>
class Clap final : public  clap::helpers::Plugin<clap::helpers::MisbehaviourHandler::Terminate, clap::helpers::CheckingLevel::Maximal> {

protected:
    PluginType plugin_{*this};
};
