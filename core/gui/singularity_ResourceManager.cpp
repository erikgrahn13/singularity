#include "singularity_ResourceManager.h"
#include <optional>

std::unordered_map<std::string, EmbeddedResource> ResourceManager::s_resources;

void ResourceManager::registerResource(const std::string &path, const unsigned char *data, size_t size,
                                       const std::string &mimeType)
{
    s_resources[path] = {data, size, mimeType};
}

std::optional<EmbeddedResource> ResourceManager::getResource(const std::string &path)
{
    auto it = s_resources.find(path);
    if (it != s_resources.end())
    {
        return it->second;
    }
    return std::nullopt;
}

void ResourceManager::clearResources()
{
    s_resources.clear();
}
