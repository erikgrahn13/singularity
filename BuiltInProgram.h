#pragma once

#include "IParameterProvider.h"
#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

struct BuiltInProgram
{
    // IDs and ordering are persistent. Adapters may expose programs to hosts by
    // index, so existing programs must not be reordered between releases.
    std::string id;
    std::string name;
    std::string category;
    std::vector<ParameterChange> parameters;

    // Optional format-neutral plug-in data. Singularity never interprets these
    // bytes: they may contain any serialization or resource reference chosen
    // by the plug-in.
    std::vector<std::byte> data;
};

struct ProgramCollection
{
    std::string id;
    std::string name;

    // Parameters owned by this collection. Every program begins with the
    // declared parameter defaults and may override a subset of these IDs.
    std::vector<unsigned int> parameterIds;
    std::vector<BuiltInProgram> programs;
};

template<typename P>
concept HasProgramCollections = requires
{
    { P::getProgramCollections() };
};

template<typename P>
concept HandlesProgramData = requires(
    P& plugin,
    std::string_view collectionId,
    std::string_view programId,
    std::span<const std::byte> data)
{
    { plugin.loadProgramData(collectionId, programId, data) };
};
