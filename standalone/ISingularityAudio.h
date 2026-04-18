#pragma once
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <limits>
#include <functional>
#include "../utilities/SingularityQueue.h"
#include "../IParameterProvider.h"
#include "../SingularityPlugin.h"

IParameterProvider& getParameterContainer();
void setOnParameterChanged(std::function<void(int, double)> cb);

class AudioDevice {
    public:
    int id;
    std::string name; 
    int inputChannels;
    int outputChannels;
    int minBufferSize;
    int maxBufferSize;
    int preferredBufferSize;
    double sampleRate;

    std::vector<int> supportedSampleRates;
};

class ISingularityAudio
{
    public:
    static std::vector<std::string> backends;

    static std::unique_ptr<ISingularityAudio> createSingularityAudio();

    virtual ~ISingularityAudio() = default;
    virtual std::vector<AudioDevice> probeDevices() const = 0;

    // Called from GUI thread: queues a parameter change for the audio thread
    void pushParameterChange(int id, double value)
    {
        _paramChanges.push({id, value});
    }

    protected:
    ISingularityAudio(std::string type)
    {
        backends.push_back(type);
        mPlugin = createPlugin();
    }

    // Called at the top of each audio callback: drains queue into local RT-safe array
    void processParameterChanges()
    {
        _currentChanges.clear();
        ParameterChange change;
        while (_paramChanges.pop(change)) {
            _params[change.id] = change.value;
            _currentChanges.add(change.id, change.value);
        }
    }

    // Stack-allocated list of changes for the current buffer — passed to process()
    class ParameterChangeList : public IParameterChanges {
        ParameterChange _changes[256];
        int _count = 0;
    public:
        void clear() { _count = 0; }
        void add(int id, double value) { if (_count < 256) _changes[_count++] = {id, value}; }
        int getCount() const override { return _count; }
        ParameterChange get(int index) const override { return _changes[index]; }
    };

    double _params[256]{};
    ParameterChangeList _currentChanges;
    std::unique_ptr<SingularityPlugin> mPlugin;

    private:
    SingularityQueue<ParameterChange, 256> _paramChanges;
};
