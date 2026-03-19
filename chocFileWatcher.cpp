#include "chocFileWatcher.h"
#include "choc/platform/choc_FileWatcher.h"

struct ChocFileWatcher::Impl {
    choc::file::Watcher watcher;

        Impl(std::filesystem::path path, std::function<void(const std::string&)> onChange)
        : watcher(std::move(path), [onChange](const choc::file::Watcher::Event& e) {
            onChange(e.file.string());
        })
    {}
};

ChocFileWatcher::ChocFileWatcher(std::filesystem::path fileOrFolderToWatch, std::function<void(const std::string& filePath)> onChange)
{
    pImpl = std::make_unique<Impl>(fileOrFolderToWatch, onChange);
}

ChocFileWatcher::~ChocFileWatcher()
{
}

std::unique_ptr<IFileWatcher> createFileWatcher(const std::string &directory, std::function<void(const std::string& filePath)> onChange)
{
    return std::make_unique<ChocFileWatcher>(directory, onChange);
}