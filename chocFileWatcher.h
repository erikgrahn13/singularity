#pragma once

#include "IFileWatcher.h"
#include "choc/platform/choc_FileWatcher.h"

#include <functional>
#include <filesystem>

class ChocFileWatcher : public IFileWatcher
{
    public:
    ChocFileWatcher(std::filesystem::path fileOrFolderToWatch);
    void setCallback(std::function<void(const std::string&)> cb) override;

    ~ChocFileWatcher();

    private:
    choc::file::Watcher watcher;
    std::function<void(const std::string&)> callback_;
};