#pragma once

#include "IFileWatcher.h"

#include <functional>
#include <string>
#include <filesystem>

class ChocFileWatcher : public IFileWatcher
{
    public:
    ChocFileWatcher(std::filesystem::path fileOrFolderToWatch, std::function<void(const std::string& filePath)> onChange);
    ~ChocFileWatcher();

    private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};