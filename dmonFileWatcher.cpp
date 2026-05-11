#include "dmonFileWatcher.h"

#define DMON_IMPL
#include "dmon.h"

struct DmonFileWatcher::Impl {
    std::string mDirectory;
    std::function<void(const std::string &filePath)> mOnChange;
    dmon_watch_id watchId{};

    static void watch_callback(dmon_watch_id watch_id, dmon_action action, const char* rootdir,
    const char* filepath, const char* oldfilepath, void* user)
    {
        if (action == DMON_ACTION_MODIFY) {
            std::cout << "Modified: " << filepath << "\n";
            auto* self = static_cast<Impl*>(user);
            // if (self) {
                //     self->scriptDirty = true;
                //     if (self->onFileChanged) self->onFileChanged();
                // }
            }
     }
};

DmonFileWatcher::DmonFileWatcher(const std::string &directory, std::function<void(const std::string& filePath)> onChange)
{
    pImpl = std::make_unique<Impl>();
    pImpl->mDirectory = directory;
    pImpl->mOnChange = std::move(onChange);

    dmon_init();
    pImpl->watchId = dmon_watch(pImpl->mDirectory.c_str(), &Impl::watch_callback, DMON_WATCHFLAGS_RECURSIVE, pImpl.get());

}

DmonFileWatcher::~DmonFileWatcher()
{
    dmon_deinit();
}

std::unique_ptr<IFileWatcher> createFileWatcher(const std::string &directory, std::function<void(const std::string& filePath)> onChange)
{
    return std::make_unique<DmonFileWatcher>(directory, onChange);
}