#include "ASIO.h"
#include <asiodrivers.h>
// #include <asio.h>
#include <iostream>

extern AsioDrivers* asioDrivers;
bool loadAsioDriver(char *name);

ASIO* ASIO::instance = nullptr;

ASIO::ASIO() : ISingularityAudio("ASIO")
{
    instance = this;
    AsioDrivers tmpDriver;
    char deviceName[MAXDRVNAMELEN];
    tmpDriver.asioGetDriverName(0, deviceName, MAXDRVNAMELEN);

    strcpy(deviceName,"SSL ASIO Driver 1");
    auto status = loadAsioDriver(deviceName);
    if(status == 0)
    {
        std::cout << "probeDriver: Failed to load ASIO driver" << std::endl;
    }
    else
    {
        std::cout << "probeDriver: Successfully loaded ASIO driver" << std::endl;
    }

    asioDrivers->getCurrentDriverName(deviceName);
    auto index = asioDrivers->getCurrentDriverIndex();

    std::cout << "ERIK1: " << index << ", "  << deviceName << std::endl;

    if(ASIOInit(&driverInfo) != ASE_OK)
    {
        throw std::runtime_error(driverInfo.errorMessage);
    }

    long granularity;
    ASIOGetChannels(&inputChannels, &outputChannels);
    printf ("ASIOGetChannels (inputs: %ld, outputs: %ld);\n", inputChannels, outputChannels);

    ASIOGetBufferSize(&minSize, &maxSize, &preferredSize, &granularity);
    printf ("ASIOGetBufferSize (min: %ld, max: %ld, preferred: %ld, granularity: %ld);\n",
        minSize, maxSize, preferredSize, granularity);

    ASIOGetSampleRate(&sampleRate);
    printf ("ASIOGetSampleRate (sampleRate: %f);\n", sampleRate);

    
    if(ASIOOutputReady() == ASE_OK)
        postOutput = true;
    else
        postOutput = false;
    printf ("ASIOOutputReady(); - %s\n", postOutput ? "Supported" : "Not supported");


    asioCallbacks.bufferSwitch = &bufferSwitch;
    asioCallbacks.sampleRateDidChange = &sampleRateDidChange;
    asioCallbacks.asioMessage = &asioMessage;
    asioCallbacks.bufferSwitchTimeInfo = &bufferSwitchTimeInfo;



    // create buffers for all inputs and outputs of the card with the 
	// preferredSize from ASIOGetBufferSize() as buffer size
	long i;
	ASIOError result;

	// fill the bufferInfos from the start without a gap
	ASIOBufferInfo *info = bufferInfos;

	// prepare inputs (Though this is not necessaily required, no opened inputs will work, too
	if (inputChannels > kMaxInputChannels)
		inputBuffers = kMaxInputChannels;
	else
		inputBuffers = inputChannels;
	for(i = 0; i < inputBuffers; i++, info++)
	{
		info->isInput = ASIOTrue;
		info->channelNum = i;
		info->buffers[0] = info->buffers[1] = 0;
	}

	// prepare outputs
	if (outputChannels > kMaxOutputChannels)
		outputBuffers = kMaxOutputChannels;
	else
		outputBuffers = outputChannels;
	for(i = 0; i < outputBuffers; i++, info++)
	{
		info->isInput = ASIOFalse;
		info->channelNum = i;
		info->buffers[0] = info->buffers[1] = 0;
	}

	// create and activate buffers
	result = ASIOCreateBuffers(bufferInfos,
		inputBuffers + outputBuffers,
		preferredSize, &asioCallbacks);
	if (result == ASE_OK)
	{
		// now get all the buffer details, sample word length, name, word clock group and activation
		for (i = 0; i < inputBuffers + outputBuffers; i++)
		{
			channelInfos[i].channel = bufferInfos[i].channelNum;
			channelInfos[i].isInput = bufferInfos[i].isInput;
			result = ASIOGetChannelInfo(&channelInfos[i]);
			if (result != ASE_OK)
				break;
		}

		if (result == ASE_OK)
		{
			// get the input and output latencies
			// Latencies often are only valid after ASIOCreateBuffers()
			// (input latency is the age of the first sample in the currently returned audio block)
			// (output latency is the time the first sample in the currently returned audio block requires to get to the output)
			result = ASIOGetLatencies(&inputLatency, &outputLatency);
			if (result == ASE_OK)
				printf ("ASIOGetLatencies (input: %d, output: %d);\n", inputLatency, outputLatency);
		}
	}

    if (ASIOStart() == ASE_OK)
    {
        // Now all is up and running
        fprintf (stdout, "\nASIO Driver started succefully.\n\n");
    }
}

ASIO::~ASIO()
{
    ASIOStop();
    ASIODisposeBuffers();
    ASIOExit();
    asioDrivers->removeCurrentDriver();
}

std::vector<AudioDevice> ASIO::probeDevices() const
{
    AsioDrivers tmpDrivers;
    auto numDevices = tmpDrivers.asioGetNumDev();

    std::vector<AudioDevice> devices;
    char deviceName[MAXDRVNAMELEN];

    for (int i = 0; i < numDevices; ++i)
    {
        tmpDrivers.asioGetDriverName(i, deviceName, MAXDRVNAMELEN);
        AudioDevice d;
        d.id = i;
        d.name = deviceName;
        devices.push_back(d);
    }

    return devices;
}

#if NATIVE_INT64
    #define ASIO64toDouble(a) (a)
#else
    static const double twoRaisedTo32 = 4294967296.;
    #define ASIO64toDouble(a) ((a).lo + (a).hi * twoRaisedTo32)
#endif

ASIOTime* ASIO::bufferSwitchTimeInfo(ASIOTime* timeInfo, long index, ASIOBool processNow)
{
    instance->tInfo = *timeInfo;

    if (timeInfo->timeInfo.flags & kSystemTimeValid)
        instance->nanoSeconds = ASIO64toDouble(timeInfo->timeInfo.systemTime);
    else
        instance->nanoSeconds = 0;

    if (timeInfo->timeInfo.flags & kSamplePositionValid)
        instance->samples = ASIO64toDouble(timeInfo->timeInfo.samplePosition);
    else
        instance->samples = 0;

#if WINDOWS && _DEBUG
	// a few debug messages for the Windows device driver developer
	// tells you the time when driver got its interrupt and the delay until the app receives
	// the event notification.
	static double last_samples = 0;
	char tmp[128];
	sprintf (tmp, "diff: %d / %d ms / %d ms / %d samples                 \n", instance.sysRefTime - (long)(instance.nanoSeconds / 1000000.0), instance.sysRefTime, (long)(instance.nanoSeconds / 1000000.0), (long)(instance.samples - last_samples));
	OutputDebugString (tmp);
	last_samples = instance.samples;
#endif

    long buffSize = instance->preferredSize;

    for (int i = 0; i < instance->inputBuffers + instance->outputBuffers; i++)
    {
        if (!instance->bufferInfos[i].isInput)
        {
            // passthrough: output channel N uses input channel N as source (if it exists)
            int outChan = i - instance->inputBuffers;
            void* src = (outChan < instance->inputBuffers)
                ? instance->bufferInfos[outChan].buffers[index]
                : nullptr;
            void* dst = instance->bufferInfos[i].buffers[index];

            switch (instance->channelInfos[i].type)
            {
            case ASIOSTInt16LSB:
                if (src) memcpy(dst, src, buffSize * 2); else memset(dst, 0, buffSize * 2);
                break;
            case ASIOSTInt24LSB:        // used for 20 bits as well
                if (src) memcpy(dst, src, buffSize * 3); else memset(dst, 0, buffSize * 3);
                break;
            case ASIOSTInt32LSB:
                if (src) memcpy(dst, src, buffSize * 4); else memset(dst, 0, buffSize * 4);
                break;
            case ASIOSTFloat32LSB:      // IEEE 754 32 bit float, as found on Intel x86 architecture
                if (src) memcpy(dst, src, buffSize * 4); else memset(dst, 0, buffSize * 4);
                break;
            case ASIOSTFloat64LSB:      // IEEE 754 64 bit double float, as found on Intel x86 architecture
                if (src) memcpy(dst, src, buffSize * 8); else memset(dst, 0, buffSize * 8);
                break;

            // these are used for 32 bit data buffer, with different alignment of the data inside
            case ASIOSTInt32LSB16:      // 32 bit data with 16 bit alignment
            case ASIOSTInt32LSB18:      // 32 bit data with 18 bit alignment
            case ASIOSTInt32LSB20:      // 32 bit data with 20 bit alignment
            case ASIOSTInt32LSB24:      // 32 bit data with 24 bit alignment
                if (src) memcpy(dst, src, buffSize * 4); else memset(dst, 0, buffSize * 4);
                break;

            case ASIOSTInt16MSB:
                if (src) memcpy(dst, src, buffSize * 2); else memset(dst, 0, buffSize * 2);
                break;
            case ASIOSTInt24MSB:        // used for 20 bits as well
                if (src) memcpy(dst, src, buffSize * 3); else memset(dst, 0, buffSize * 3);
                break;
            case ASIOSTInt32MSB:
                if (src) memcpy(dst, src, buffSize * 4); else memset(dst, 0, buffSize * 4);
                break;
            case ASIOSTFloat32MSB:      // IEEE 754 32 bit float, as found on Intel x86 architecture
                if (src) memcpy(dst, src, buffSize * 4); else memset(dst, 0, buffSize * 4);
                break;
            case ASIOSTFloat64MSB:      // IEEE 754 64 bit double float, as found on Intel x86 architecture
                if (src) memcpy(dst, src, buffSize * 8); else memset(dst, 0, buffSize * 8);
                break;

            // these are used for 32 bit data buffer, with different alignment of the data inside
            case ASIOSTInt32MSB16:      // 32 bit data with 16 bit alignment
            case ASIOSTInt32MSB18:      // 32 bit data with 18 bit alignment
            case ASIOSTInt32MSB20:      // 32 bit data with 20 bit alignment
            case ASIOSTInt32MSB24:      // 32 bit data with 24 bit alignment
                if (src) memcpy(dst, src, buffSize * 4); else memset(dst, 0, buffSize * 4);
                break;
            }
        }
    }

    if (instance->postOutput)
        ASIOOutputReady();

    return nullptr;
}

void ASIO::bufferSwitch(long doubleBufferIndex, ASIOBool directProcess)
{
    ASIOTime timeInfo;
    memset(&timeInfo, 0, sizeof(timeInfo));

    if(ASIOGetSamplePosition(&timeInfo.timeInfo.samplePosition, &timeInfo.timeInfo.systemTime) == ASE_OK)
    {
        timeInfo.timeInfo.flags = kSystemTimeValid | kSamplePositionValid;
    }
    bufferSwitchTimeInfo(&timeInfo, doubleBufferIndex, directProcess);
}

void ASIO::sampleRateDidChange(ASIOSampleRate sRate)
{
    // From the ASIO SDK:
    // do whatever you need to do if the sample rate changed
    // usually this only happens during external sync.
    // Audio processing is not stopped by the driver, actual sample rate
    // might not have even changed, maybe only the sample rate status of an
    // AES/EBU or S/PDIF digital input at the audio device.
    // You might have to update time/sample related conversion routines, etc.
}

long ASIO::asioMessage(long selector, long value, void* message, double *opt)
{
    int returnValue{0};
    switch (selector)
    {
    case kAsioSelectorSupported:
        if(value == kAsioResetRequest || value == kAsioEngineVersion || value == kAsioResetRequest
            || value == kAsioLatenciesChanged || value == kAsioSupportsInputMonitor || value == kAsioOverload) 
            returnValue = 1;
        break;
    case kAsioEngineVersion:
        std::cout << "AsioEngineVersion" << std::endl;
        returnValue = 2;
        break;
    case kAsioResetRequest:
        std::cout << "AsioResetRequest" << std::endl;
        returnValue = 1;
        break;
    case kAsioBufferSizeChange:
        std::cout << "AsioBufferSizeChange" << std::endl;
        break;
    case kAsioResyncRequest:
        std::cout << "AsioResyncRequest" << std::endl;
        break;
    case kAsioLatenciesChanged:
        std::cout << "AsioLatenciesChanged" << std::endl;
        break;
    case kAsioSupportsTimeInfo:
        std::cout << "AsioSupportsTimeInfo" << std::endl;
        returnValue = 1;
        break;
    case kAsioSupportsTimeCode:
        std::cout << "AsioSupportsTimeCode" << std::endl;
        break;
    case kAsioMMCCommand:
        std::cout << "AsioMMCCommand" << std::endl;
        break;
    case kAsioSupportsInputMonitor:
        std::cout << "AsioSupportsInputMonitor" << std::endl;
        break;
    case kAsioSupportsInputGain:
        std::cout << "AsioSupportsInputGain" << std::endl;
        break;
    case kAsioSupportsOutputGain:
        std::cout << "AsioSupportsOutputGain" << std::endl;
        break;
    case kAsioSupportsOutputMeter:
        std::cout << "AsioSupportsOutputMeter" << std::endl;
        break;
    case kAsioOverload:
        std::cout << "AsioOverload" << std::endl;
        break;
    default:
        break;
    }
    return returnValue;
}

