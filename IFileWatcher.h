#pragma once

#include <memory>
#include <string>
#include <functional>

class IFileWatcher {
    public:
    static std::unique_ptr<IFileWatcher> createFileWatcher(const std::string &directory);
    virtual void setCallback(std::function<void(const std::string&)> cb) = 0;
    virtual ~IFileWatcher() = default;
};