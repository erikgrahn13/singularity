#include "chocFileWatcher.h"

ChocFileWatcher::ChocFileWatcher(std::filesystem::path fileOrFolderToWatch)
     : watcher(fileOrFolderToWatch, [this](const choc::file::Watcher::Event& e)
    {
        if (callback_)
            callback_(e.file.string()); // adjust if needed
    })
{
}

void ChocFileWatcher::setCallback(std::function<void(const std::string &)> cb)
{
    callback_ = std::move(cb);
}

ChocFileWatcher::~ChocFileWatcher()
{
}

std::unique_ptr<IFileWatcher> IFileWatcher::createFileWatcher(const std::string &directory)
{
    return std::make_unique<ChocFileWatcher>(directory);
}