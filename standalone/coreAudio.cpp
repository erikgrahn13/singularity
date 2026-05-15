#include "coreAudio.h"
#include PLUGIN_CLASS_HEADER
#include <print>

template<typename PluginType>
CoreAudio<PluginType>::CoreAudio() : ISingularityAudio<PluginType>()
{
    OSStatus err = noErr;

    // 1. Find and create AUHAL
    AudioComponentDescription desc = {};
    desc.componentType         = kAudioUnitType_Output;
    desc.componentSubType      = kAudioUnitSubType_HALOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;

    AudioComponent comp = AudioComponentFindNext(nullptr, &desc);
    if (!comp) { fprintf(stderr, "Could not find AUHAL component\n"); return; }

    err = AudioComponentInstanceNew(comp, &mAudioUnit);
    checkErr(err);

    // 2. EnableIO — must happen before Initialize
    UInt32 one = 1, zero = 0;
    err = AudioUnitSetProperty(mAudioUnit, kAudioOutputUnitProperty_EnableIO,
                               kAudioUnitScope_Input,  1, &one,  sizeof(one));
    checkErr(err);
    err = AudioUnitSetProperty(mAudioUnit, kAudioOutputUnitProperty_EnableIO,
                               kAudioUnitScope_Output, 0, &one, sizeof(one));
    checkErr(err);

    // 3. Set device — same device for input and output (pro audio interface)
    AudioDeviceID device = getDefaultInputDevice();
    err = AudioUnitSetProperty(mAudioUnit, kAudioOutputUnitProperty_CurrentDevice,
                               kAudioUnitScope_Global, 0, &device, sizeof(device));
    checkErr(err);

    // 4. Set output render callback — pulls input directly inside the callback
    AURenderCallbackStruct outputCb = { OutputRenderCallBack, this };
    err = AudioUnitSetProperty(mAudioUnit, kAudioUnitProperty_SetRenderCallback,
                               kAudioUnitScope_Input, 0, &outputCb, sizeof(outputCb));
    checkErr(err);

    // 5. Initialize and start
    err = AudioUnitInitialize(mAudioUnit);
    checkErr(err);

    err = AudioOutputUnitStart(mAudioUnit);
    checkErr(err);

    std::println("CoreAudio initialized on device {}", device);
}

template<typename PluginType>
CoreAudio<PluginType>::~CoreAudio()
{
    if (mAudioUnit) {
        AudioOutputUnitStop(mAudioUnit);
        AudioUnitUninitialize(mAudioUnit);
        AudioComponentInstanceDispose(mAudioUnit);
    }
}

// Single callback: AUHAL asks for output, we pull input directly and run DSP in-place
template<typename PluginType>
OSStatus CoreAudio<PluginType>::OutputRenderCallBack(void *inRefCon,
                                          AudioUnitRenderActionFlags *ioActionFlags,
                                          const AudioTimeStamp *inTimeStamp,
                                          UInt32 inBusNumber,
                                          UInt32 inNumberFrames,
                                          AudioBufferList *ioData)
{
    CoreAudio<PluginType> *self = static_cast<CoreAudio<PluginType>*>(inRefCon);

    // Pull input from hardware directly into ioData (element 1 = input bus)
    OSStatus err = AudioUnitRender(self->mAudioUnit, ioActionFlags, inTimeStamp,
                                   1, inNumberFrames, ioData);
    checkErr(err);

    // TODO: run DSP on ioData in-place here

    return err;
}

template<typename PluginType>
std::vector<AudioDevice> CoreAudio<PluginType>::probeDevices() const
{
    uint32_t propSize;

    AudioObjectPropertyAddress theAddress = {
        .mSelector = kAudioHardwarePropertyDevices,
        .mScope    = kAudioObjectPropertyScopeGlobal,
        .mElement  = kAudioObjectPropertyElementMain
    };

    AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &theAddress, 0, nullptr, &propSize);
    int nDevices = propSize / sizeof(AudioDeviceID);
    std::vector<AudioDeviceID> deviceIDs(nDevices);
    AudioObjectGetPropertyData(kAudioObjectSystemObject, &theAddress, 0, nullptr, &propSize, deviceIDs.data());

    std::vector<AudioDevice> devices;
    for (int i = 0; i < nDevices; ++i) {
        AudioDevice dev;
        dev.id             = deviceIDs[i];
        dev.name           = getDeviceName(deviceIDs[i]);
        dev.inputChannels  = countChannels(deviceIDs[i], true);
        dev.outputChannels = countChannels(deviceIDs[i], false);
        devices.push_back(dev);
        std::println("[{}] {} (in: {}, out: {})", dev.id, dev.name, dev.inputChannels, dev.outputChannels);
    }
    return devices;
}

template<typename PluginType>
std::string CoreAudio<PluginType>::getDeviceName(AudioDeviceID deviceID) const
{
    AudioObjectPropertyAddress addr = {
        .mSelector = kAudioObjectPropertyName,
        .mScope    = kAudioObjectPropertyScopeGlobal,
        .mElement  = kAudioObjectPropertyElementMain
    };

    CFStringRef cfName = nullptr;
    uint32_t size = sizeof(cfName);
    verify_noerr(AudioObjectGetPropertyData(deviceID, &addr, 0, nullptr, &size, &cfName));
    if (!cfName) return {};

    char buf[256];
    CFStringGetCString(cfName, buf, sizeof(buf), kCFStringEncodingUTF8);
    CFRelease(cfName);
    return buf;
}

template<typename PluginType>
int CoreAudio<PluginType>::countChannels(AudioDeviceID deviceID, bool isInput) const
{
    AudioObjectPropertyAddress addr = {
        .mSelector = kAudioDevicePropertyStreamConfiguration,
        .mScope    = isInput ? kAudioDevicePropertyScopeInput : kAudioDevicePropertyScopeOutput,
        .mElement  = 0
    };

    uint32_t propSize = 0;
    if (AudioObjectGetPropertyDataSize(deviceID, &addr, 0, nullptr, &propSize)) return 0;

    std::vector<std::byte> buffer(propSize);
    auto *bufferList = reinterpret_cast<AudioBufferList*>(buffer.data());
    if (AudioObjectGetPropertyData(deviceID, &addr, 0, nullptr, &propSize, bufferList)) return 0;

    int result = 0;
    for (uint32_t i = 0; i < bufferList->mNumberBuffers; ++i)
        result += bufferList->mBuffers[i].mNumberChannels;
    return result;
}

template<typename PluginType>
AudioDeviceID CoreAudio<PluginType>::getDefaultInputDevice() const
{
    AudioDeviceID deviceID = kAudioObjectUnknown;
    uint32_t size = sizeof(deviceID);
    AudioObjectPropertyAddress addr = {
        .mSelector = kAudioHardwarePropertyDefaultInputDevice,
        .mScope    = kAudioObjectPropertyScopeGlobal,
        .mElement  = kAudioObjectPropertyElementMain
    };
    AudioObjectGetPropertyData(kAudioObjectSystemObject, &addr, 0, nullptr, &size, &deviceID);
    return deviceID;
}

template<typename PluginType>
AudioDeviceID CoreAudio<PluginType>::getDefaultOutputDevice() const
{
    AudioDeviceID deviceID = kAudioObjectUnknown;
    uint32_t size = sizeof(deviceID);
    AudioObjectPropertyAddress addr = {
        .mSelector = kAudioHardwarePropertyDefaultOutputDevice,
        .mScope    = kAudioObjectPropertyScopeGlobal,
        .mElement  = kAudioObjectPropertyElementMain
    };
    AudioObjectGetPropertyData(kAudioObjectSystemObject, &addr, 0, nullptr, &size, &deviceID);
    return deviceID;
}

template class CoreAudio<PLUGIN_CLASS>;