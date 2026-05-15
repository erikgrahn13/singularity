#pragma once

#include "ISingularityAudio.h"
#include <asiosys.h>
#include <asio.h>

template<typename PluginType>
class ASIO : public ISingularityAudio<PluginType>
{
    public:
    ASIO();

    ~ASIO();
    std::vector<AudioDevice> probeDevices() const override;

    private:
    
    	// ASIOInit()
	ASIODriverInfo driverInfo;

	// ASIOGetChannels()
	long           inputChannels;
	long           outputChannels;

	// ASIOGetBufferSize()
	long           minSize;
	long           maxSize;
	long           preferredSize;
	long           granularity;

	// ASIOGetSampleRate()
	ASIOSampleRate sampleRate;

	// ASIOOutputReady()
	bool           postOutput;

	// ASIOGetLatencies ()
	long           inputLatency;
	long           outputLatency;

    static constexpr int kMaxInputChannels{32};
    static constexpr int kMaxOutputChannels{32};
	// ASIOCreateBuffers ()
	long inputBuffers;	// becomes number of actual created input buffers
	long outputBuffers;	// becomes number of actual created output buffers
	ASIOBufferInfo bufferInfos[kMaxInputChannels + kMaxOutputChannels]; // buffer info's

	// ASIOGetChannelInfo()
	ASIOChannelInfo channelInfos[kMaxInputChannels + kMaxOutputChannels]; // channel info's
	// The above two arrays share the same indexing, as the data in them are linked together

	// Information from ASIOGetSamplePosition()
	// data is converted to double floats for easier use, however 64 bit integer can be used, too
	double         nanoSeconds;
	double         samples;
	double         tcSamples;	// time code samples

	// bufferSwitchTimeInfo()
	ASIOTime       tInfo;			// time info state
	unsigned long  sysRefTime;      // system reference time, when bufferSwitch() was called

	// Signal the end of processing in this example
	bool           stopped;

    ASIOCallbacks asioCallbacks;
    static void bufferSwitch(long index, ASIOBool processNow);
    static void sampleRateDidChange(ASIOSampleRate sRate);
    static long asioMessage(long selector, long value, void* message, double *opt);
    static ASIOTime *bufferSwitchTimeInfo(ASIOTime *timeInfo, long index, ASIOBool processNow);

    // be able to reach this instace in the static callback functions
    static ASIO* instance;
};