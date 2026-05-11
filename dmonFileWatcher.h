#pragma once

#include "IFileWatcher.h"


//TODO: remove
#include <iostream>
#include <functional>



class DmonFileWatcher : public IFileWatcher
{
    public:
    DmonFileWatcher(const std::string &directory, std::function<void(const std::string& filePath)> onChange);
    ~DmonFileWatcher();

    private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};
