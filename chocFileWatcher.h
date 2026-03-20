#pragma once

#include "IFileWatcher.h"
#include "choc/platform/choc_FileWatcher.h"

#include <functional>
#include <filesystem>

class ChocFileWatcher : public IFileWatcher
{
    public:
    ChocFileWatcher(std::filesystem::path fileOrFolderToWatch, std::function<void(const choc::file::Watcher::Event&)> onChange);
    ~ChocFileWatcher();

    private:
    choc::file::Watcher watcher;
};