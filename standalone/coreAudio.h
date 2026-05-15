#pragma once
#include "CoreAudio/CoreAudio.h"

#include <string_view>
#include <AudioUnit/AudioUnit.h>
#include <CoreFoundation/CoreFoundation.h>
#include <vector>
#include "ISingularityAudio.h"

#ifndef verify_noerr
    #define verify_noerr(errorCode) do { OSStatus __err = (errorCode); if (__err != noErr) { fprintf(stderr, "Error %d at %s:%d\n", (int)__err, __FILE__, __LINE__); } } while (0)
#endif

#define checkErr(err) do { if (err) { fprintf(stderr, "CoreAudio Error: %d at %s:%d\n", (int)err, __FILE__, __LINE__); } } while(0)

template<typename PluginType>
class CoreAudio : public ISingularityAudio<PluginType>
{
    public:
    CoreAudio();
    ~CoreAudio();

    std::vector<AudioDevice> probeDevices() const override;

    private:
    std::string getDeviceName(AudioDeviceID deviceID) const;
    int countChannels(AudioDeviceID audioDeviceID, bool isInput) const;

    AudioDeviceID getDefaultInputDevice() const;
    AudioDeviceID getDefaultOutputDevice() const;

    AudioUnit mAudioUnit{nullptr};

    static OSStatus OutputRenderCallBack(void *inRefCon,
                        AudioUnitRenderActionFlags *ioActionFlags,
                        const AudioTimeStamp *inTimeStamp,
                        UInt32 inBusNumber,
                        UInt32 inNumberFrames,
                        AudioBufferList *ioData);
}; 