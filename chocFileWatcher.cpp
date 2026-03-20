#include "chocFileWatcher.h"

ChocFileWatcher::ChocFileWatcher(std::filesystem::path fileOrFolderToWatch, std::function<void(const choc::file::Watcher::Event&)> onChange)
 : watcher(std::move(fileOrFolderToWatch), onChange)
{
}

ChocFileWatcher::~ChocFileWatcher()
{
}

std::unique_ptr<IFileWatcher> createFileWatcher(const std::string &directory, std::function<void(const std::string& filePath)> onChange)
{
    return std::make_unique<ChocFileWatcher>(directory, [onChange](const choc::file::Watcher::Event& e) {
        onChange(e.file.string());
    });
}