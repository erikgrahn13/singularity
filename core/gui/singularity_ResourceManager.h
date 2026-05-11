#pragma once
#include <optional>
#include <string>
#include <unordered_map>

struct EmbeddedResource
{
    const unsigned char *data;
    size_t size;
    std::string mimeType;
};

class ResourceManager
{
  public:
    static void registerResource(const std::string &path, const unsigned char *data, size_t size,
                                 const std::string &mimeType);
    static std::optional<EmbeddedResource> getResource(const std::string &path);
    static void clearResources();

  private:
    static std::unordered_map<std::string, EmbeddedResource> s_resources;
};

// Struct to auto-register resources at static initialization time
struct ResourceRegistrar
{
    ResourceRegistrar(const std::string &path, const unsigned char *data, size_t size, const std::string &mimeType)
    {
        ResourceManager::registerResource(path, data, size, mimeType);
    }
};

// Macro to embed and auto-register a resource
#define EMBED_RESOURCE(var_name, path, mime_type)                                                                      \
    static const ResourceRegistrar var_name##_registrar(path, var_name##_data, var_name##_size, mime_type);
