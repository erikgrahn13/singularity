#include "WASAPI.h"
#include PLUGIN_CLASS_HEADER
#include <windows.h>
#include <audioclient.h>
#include <avrt.h>
#include <algorithm>
#include <cmath>
#include <exception>
#include <iostream>
#include <iterator>
#include <ks.h>
#include <ksmedia.h>
#include <mmdeviceapi.h>
#include <mmreg.h>
#include <propkeydef.h>
#include <functiondiscoverykeys_devpkey.h>
#include <print>
#include <propsys.h>
#include <propvarutil.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <wrl/client.h>

namespace
{
using Microsoft::WRL::ComPtr;

constexpr double referenceTimePerSecond = 10'000'000.0;
constexpr REFERENCE_TIME requestedBufferDuration = 200'000;
constexpr int stereoChannels = 2;
// Keep the capture FIFO centred despite independent input/output device clocks.
constexpr double maximumCaptureRateCorrection = 0.0025;
constexpr double captureRateCorrectionGain = 0.01;
constexpr double captureRateSmoothing = 0.05;

void throwIfFailed(HRESULT result, const char* operation)
{
    if (SUCCEEDED(result))
        return;

    std::ostringstream message;
    message << operation << " failed with HRESULT 0x"
            << std::hex << static_cast<unsigned long>(result);
    throw std::runtime_error(message.str());
}

class WinHandle
{
public:
    WinHandle() = default;
    explicit WinHandle(HANDLE handle) : handle_(handle) {}

    ~WinHandle()
    {
        if (handle_ != nullptr)
            CloseHandle(handle_);
    }

    WinHandle(const WinHandle&) = delete;
    WinHandle& operator=(const WinHandle&) = delete;

    HANDLE get() const { return handle_; }
    explicit operator bool() const { return handle_ != nullptr; }

private:
    HANDLE handle_{nullptr};
};

class MmcssRegistration
{
public:
    MmcssRegistration()
    {
        DWORD taskIndex = 0;
        handle_ = AvSetMmThreadCharacteristicsW(L"Pro Audio", &taskIndex);
    }

    ~MmcssRegistration()
    {
        if (handle_ != nullptr)
            AvRevertMmThreadCharacteristics(handle_);
    }

    bool succeeded() const { return handle_ != nullptr; }

private:
    HANDLE handle_{nullptr};
};

class StereoRingBuffer
{
public:
    StereoRingBuffer(
        std::size_t capacityInFrames,
        std::size_t targetFillFrames)
        : samples_(capacityInFrames * stereoChannels),
          capacityInFrames_(capacityInFrames),
          targetFillFrames_(std::clamp<std::size_t>(
              targetFillFrames, 2, capacityInFrames - 1))
    {
    }

    void clear()
    {
        readFrame_ = 0;
        writeFrame_ = 0;
        availableFrames_ = 0;
        fractionalReadPosition_ = 0.0;
        sourceFramesPerOutputFrame_ = 1.0;
        primed_ = false;
    }

    void push(const float* interleaved, std::size_t frames, bool silent)
    {
        for (std::size_t frame = 0; frame < frames; ++frame)
        {
            if (availableFrames_ == capacityInFrames_)
            {
                readFrame_ = (readFrame_ + 1) % capacityInFrames_;
                --availableFrames_;
            }

            const auto destination = writeFrame_ * stereoChannels;
            samples_[destination] = silent ? 0.0f : interleaved[frame * stereoChannels];
            samples_[destination + 1] = silent ? 0.0f : interleaved[frame * stereoChannels + 1];
            writeFrame_ = (writeFrame_ + 1) % capacityInFrames_;
            ++availableFrames_;
        }
    }

    void pop(float* left, float* right, std::size_t frames)
    {
        // Establish enough capture latency to absorb normal event-scheduling
        // jitter before allowing the render stream to consume input.
        if (!primed_)
        {
            if (availableFrames_ < targetFillFrames_)
            {
                std::fill_n(left, frames, 0.0f);
                std::fill_n(right, frames, 0.0f);
                return;
            }
            primed_ = true;
        }

        // A fuller FIFO consumes capture slightly faster; a shallower FIFO
        // consumes it slightly slower. Linear interpolation makes the small
        // rate changes continuous instead of dropping or duplicating frames.
        const double fillError =
            (static_cast<double>(availableFrames_)
             - static_cast<double>(targetFillFrames_))
            / static_cast<double>(targetFillFrames_);
        const double desiredRate = 1.0 + std::clamp(
            fillError * captureRateCorrectionGain,
            -maximumCaptureRateCorrection,
            maximumCaptureRateCorrection);
        sourceFramesPerOutputFrame_ += captureRateSmoothing
            * (desiredRate - sourceFramesPerOutputFrame_);

        for (std::size_t frame = 0; frame < frames; ++frame)
        {
            if (availableFrames_ < 2)
            {
                std::fill_n(left + frame, frames - frame, 0.0f);
                std::fill_n(right + frame, frames - frame, 0.0f);
                clear();
                return;
            }

            const auto nextFrame = (readFrame_ + 1) % capacityInFrames_;
            const auto source = readFrame_ * stereoChannels;
            const auto nextSource = nextFrame * stereoChannels;
            const auto fraction = static_cast<float>(fractionalReadPosition_);
            left[frame] = samples_[source]
                + fraction * (samples_[nextSource] - samples_[source]);
            right[frame] = samples_[source + 1]
                + fraction * (samples_[nextSource + 1] - samples_[source + 1]);

            fractionalReadPosition_ += sourceFramesPerOutputFrame_;
            const auto framesToConsume = std::min(
                static_cast<std::size_t>(std::floor(fractionalReadPosition_)),
                availableFrames_ - 1);
            readFrame_ = (readFrame_ + framesToConsume) % capacityInFrames_;
            availableFrames_ -= framesToConsume;
            fractionalReadPosition_ -= static_cast<double>(framesToConsume);
        }
    }

private:
    std::vector<float> samples_;
    std::size_t capacityInFrames_;
    std::size_t targetFillFrames_;
    std::size_t readFrame_{0};
    std::size_t writeFrame_{0};
    std::size_t availableFrames_{0};
    double fractionalReadPosition_{0.0};
    double sourceFramesPerOutputFrame_{1.0};
    bool primed_{false};
};

class ComApartment
{
public:
    ComApartment()
        : result_(CoInitializeEx(nullptr, COINIT_MULTITHREADED))
    {
    }

    ~ComApartment()
    {
        if (SUCCEEDED(result_))
            CoUninitialize();
    }

    bool isAvailable() const
    {
        return SUCCEEDED(result_) || result_ == RPC_E_CHANGED_MODE;
    }

private:
    HRESULT result_;
};

std::string utf8FromWide(const wchar_t* text)
{
    if (text == nullptr || *text == L'\0')
        return {};

    const int requiredSize = WideCharToMultiByte(
        CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (requiredSize <= 0)
        return {};

    std::string result(static_cast<std::size_t>(requiredSize), '\0');
    WideCharToMultiByte(
        CP_UTF8, 0, text, -1, result.data(), requiredSize, nullptr, nullptr);
    result.pop_back();
    return result;
}

std::string getDeviceFriendlyName(IMMDevice& endpoint)
{
    ComPtr<IPropertyStore> properties;
    if (FAILED(endpoint.OpenPropertyStore(STGM_READ, &properties)))
        return {};

    PROPVARIANT friendlyName;
    PropVariantInit(&friendlyName);
    std::string name;
    if (SUCCEEDED(properties->GetValue(PKEY_Device_FriendlyName, &friendlyName))
        && friendlyName.vt == VT_LPWSTR)
    {
        name = utf8FromWide(friendlyName.pwszVal);
    }
    PropVariantClear(&friendlyName);
    return name;
}

AudioDevice describeRenderDevice(IMMDevice& endpoint, bool isDefault)
{
    AudioDevice device{};
    device.isDefault = isDefault;
    device.name = getDeviceFriendlyName(endpoint);

    if (device.name.empty())
        device.name = "Unknown WASAPI output";

    ComPtr<IAudioClient> audioClient;
    if (FAILED(endpoint.Activate(
            __uuidof(IAudioClient),
            CLSCTX_ALL,
            nullptr,
            reinterpret_cast<void**>(audioClient.GetAddressOf()))))
    {
        return device;
    }

    WAVEFORMATEX* mixFormat = nullptr;
    if (SUCCEEDED(audioClient->GetMixFormat(&mixFormat)))
    {
        device.outputChannels = mixFormat->nChannels;
        device.sampleRate = mixFormat->nSamplesPerSec;
        CoTaskMemFree(mixFormat);
    }

    REFERENCE_TIME defaultPeriod = 0;
    REFERENCE_TIME minimumPeriod = 0;
    if (device.sampleRate > 0.0
        && SUCCEEDED(audioClient->GetDevicePeriod(&defaultPeriod, &minimumPeriod)))
    {
        device.minBufferSize = static_cast<int>(
            device.sampleRate * minimumPeriod / referenceTimePerSecond);
        device.preferredBufferSize = static_cast<int>(
            device.sampleRate * defaultPeriod / referenceTimePerSecond);
    }

    return device;
}

WAVEFORMATEXTENSIBLE makeStereoFloatFormat(DWORD sampleRate)
{
    WAVEFORMATEXTENSIBLE format{};
    format.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    format.Format.nChannels = stereoChannels;
    format.Format.nSamplesPerSec = sampleRate;
    format.Format.wBitsPerSample = 32;
    format.Format.nBlockAlign = stereoChannels * sizeof(float);
    format.Format.nAvgBytesPerSec = sampleRate * format.Format.nBlockAlign;
    format.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
    format.Samples.wValidBitsPerSample = 32;
    format.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
    format.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
    return format;
}

HRESULT activateAudioClient(IMMDevice& endpoint, ComPtr<IAudioClient>& audioClient)
{
    audioClient.Reset();
    return endpoint.Activate(
        __uuidof(IAudioClient),
        CLSCTX_ALL,
        nullptr,
        reinterpret_cast<void**>(audioClient.GetAddressOf()));
}

HRESULT initializeLowLatencySharedStream(
    ComPtr<IAudioClient>& audioClient,
    const WAVEFORMATEX& format,
    UINT32& selectedPeriodFrames)
{
    ComPtr<IAudioClient3> audioClient3;
    HRESULT result = audioClient.As(&audioClient3);
    if (FAILED(result))
        return result;

    UINT32 defaultPeriodFrames = 0;
    UINT32 fundamentalPeriodFrames = 0;
    UINT32 minimumPeriodFrames = 0;
    UINT32 maximumPeriodFrames = 0;
    result = audioClient3->GetSharedModeEnginePeriod(
        &format,
        &defaultPeriodFrames,
        &fundamentalPeriodFrames,
        &minimumPeriodFrames,
        &maximumPeriodFrames);
    if (FAILED(result))
        return result;

    selectedPeriodFrames = minimumPeriodFrames;
    return audioClient3->InitializeSharedAudioStream(
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
        selectedPeriodFrames,
        &format,
        nullptr);
}
}

template<typename PluginType>
WASAPI<PluginType>::WASAPI() : ISingularityAudio<PluginType>()
{
    const auto devices = probeDevices();
    std::println("Available WASAPI output devices:");
    if (devices.empty())
        std::println("  (none found)");

    for (const auto& device : devices)
    {
        std::println(
            "  [{}] {}{} (channels: {}, sample rate: {}, preferred buffer: {})",
            device.id,
            device.name,
            device.isDefault ? " [default]" : "",
            device.outputChannels,
            device.sampleRate,
            device.preferredBufferSize);
    }

    stopEvent_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (stopEvent_ == nullptr)
        throw std::runtime_error("Failed to create WASAPI stop event");

    std::promise<void> startupPromise;
    auto startupFuture = startupPromise.get_future();
    try
    {
        audioThread_ = std::thread(
            &WASAPI<PluginType>::audioThreadMain,
            this,
            std::move(startupPromise));
        startupFuture.get();
    }
    catch (...)
    {
        SetEvent(static_cast<HANDLE>(stopEvent_));
        if (audioThread_.joinable())
            audioThread_.join();
        CloseHandle(static_cast<HANDLE>(stopEvent_));
        stopEvent_ = nullptr;
        throw;
    }
}

template<typename PluginType>
WASAPI<PluginType>::~WASAPI()
{
    if (stopEvent_ != nullptr)
        SetEvent(static_cast<HANDLE>(stopEvent_));
    if (audioThread_.joinable())
        audioThread_.join();
    if (stopEvent_ != nullptr)
        CloseHandle(static_cast<HANDLE>(stopEvent_));
}

template<typename PluginType>
void WASAPI<PluginType>::audioThreadMain(std::promise<void> startupPromise) noexcept
{
    bool startupReported = false;

    try
    {
        ComApartment comApartment;
        if (!comApartment.isAvailable())
            throw std::runtime_error("Failed to initialize COM for WASAPI audio");

        MmcssRegistration mmcssRegistration;
        if (!mmcssRegistration.succeeded())
            std::cerr << "Failed to register the WASAPI thread with MMCSS"
                      << std::endl;

        ComPtr<IMMDeviceEnumerator> enumerator;
        throwIfFailed(
            CoCreateInstance(
                __uuidof(MMDeviceEnumerator),
                nullptr,
                CLSCTX_ALL,
                IID_PPV_ARGS(enumerator.GetAddressOf())),
            "Creating the WASAPI device enumerator");

        ComPtr<IMMDevice> renderEndpoint;
        throwIfFailed(
            enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &renderEndpoint),
            "Getting the default WASAPI output");

        ComPtr<IAudioClient> renderAudioClient;
        throwIfFailed(
            activateAudioClient(*renderEndpoint.Get(), renderAudioClient),
            "Activating the WASAPI output client");

        WAVEFORMATEX* outputMixFormat = nullptr;
        throwIfFailed(
            renderAudioClient->GetMixFormat(&outputMixFormat),
            "Getting the WASAPI output format");
        const DWORD sampleRate = outputMixFormat->nSamplesPerSec;
        CoTaskMemFree(outputMixFormat);

        auto streamFormat = makeStereoFloatFormat(sampleRate);
        UINT32 renderPeriodFrames = 0;
        bool renderUsesAudioClient3 = false;
        HRESULT renderInitialization = initializeLowLatencySharedStream(
            renderAudioClient,
            streamFormat.Format,
            renderPeriodFrames);

        if (SUCCEEDED(renderInitialization))
        {
            renderUsesAudioClient3 = true;
        }
        else
        {
            std::cerr << "IAudioClient3 output initialization unavailable; "
                         "falling back to IAudioClient"
                      << std::endl;
            throwIfFailed(
                activateAudioClient(*renderEndpoint.Get(), renderAudioClient),
                "Reactivating the WASAPI output client");

            constexpr DWORD renderFlags =
                AUDCLNT_STREAMFLAGS_EVENTCALLBACK
                | AUDCLNT_STREAMFLAGS_NOPERSIST
                | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM
                | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY;

            throwIfFailed(
                renderAudioClient->Initialize(
                    AUDCLNT_SHAREMODE_SHARED,
                    renderFlags,
                    requestedBufferDuration,
                    0,
                    &streamFormat.Format,
                    nullptr),
                "Initializing the WASAPI output stream");
        }

        WinHandle renderEvent(CreateEventW(nullptr, FALSE, FALSE, nullptr));
        if (!renderEvent)
            throw std::runtime_error("Failed to create the WASAPI output event");
        throwIfFailed(
            renderAudioClient->SetEventHandle(renderEvent.get()),
            "Setting the WASAPI output event");

        UINT32 renderBufferFrames = 0;
        throwIfFailed(
            renderAudioClient->GetBufferSize(&renderBufferFrames),
            "Getting the WASAPI output buffer size");

        ComPtr<IAudioRenderClient> renderClient;
        throwIfFailed(
            renderAudioClient->GetService(
                __uuidof(IAudioRenderClient),
                reinterpret_cast<void**>(renderClient.GetAddressOf())),
            "Getting the WASAPI render client");

        WinHandle captureEvent(CreateEventW(nullptr, FALSE, FALSE, nullptr));
        ComPtr<IAudioClient> captureAudioClient;
        ComPtr<IAudioCaptureClient> captureClient;
        bool captureAvailable = false;
        bool captureUsesAudioClient3 = false;
        UINT32 capturePeriodFrames = 0;

        if constexpr (!PluginType::isInstrument)
        {
            ComPtr<IMMDevice> captureEndpoint;
            HRESULT captureResult = enumerator->GetDefaultAudioEndpoint(
                eCapture, eConsole, &captureEndpoint);

            if (SUCCEEDED(captureResult))
            {
                const auto captureName = getDeviceFriendlyName(*captureEndpoint.Get());
                std::println(
                    "Using WASAPI capture device: {}",
                    captureName.empty() ? "Unknown WASAPI input" : captureName);
            }

            if (SUCCEEDED(captureResult))
            {
                captureResult = activateAudioClient(
                    *captureEndpoint.Get(), captureAudioClient);
            }

            if (SUCCEEDED(captureResult))
            {
                captureResult = initializeLowLatencySharedStream(
                    captureAudioClient,
                    streamFormat.Format,
                    capturePeriodFrames);
                captureUsesAudioClient3 = SUCCEEDED(captureResult);
            }

            if (FAILED(captureResult) && captureEndpoint)
            {
                captureResult = activateAudioClient(
                    *captureEndpoint.Get(), captureAudioClient);
                if (SUCCEEDED(captureResult))
                {
                    constexpr DWORD captureFlags =
                        AUDCLNT_STREAMFLAGS_EVENTCALLBACK
                        | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM
                        | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY;
                    captureResult = captureAudioClient->Initialize(
                        AUDCLNT_SHAREMODE_SHARED,
                        captureFlags,
                        requestedBufferDuration,
                        0,
                        &streamFormat.Format,
                        nullptr);
                }
            }

            if (SUCCEEDED(captureResult) && !captureEvent)
                captureResult = E_OUTOFMEMORY;
            if (SUCCEEDED(captureResult) && captureEvent)
                captureResult = captureAudioClient->SetEventHandle(captureEvent.get());
            if (SUCCEEDED(captureResult))
            {
                captureResult = captureAudioClient->GetService(
                    __uuidof(IAudioCaptureClient),
                    reinterpret_cast<void**>(captureClient.GetAddressOf()));
            }

            captureAvailable = SUCCEEDED(captureResult);
            if (!captureAvailable)
            {
                captureClient.Reset();
                captureAudioClient.Reset();
                std::cerr << "WASAPI capture unavailable; effect input will be silent"
                          << std::endl;
            }
        }

        this->callPrepare(
            static_cast<double>(sampleRate),
            static_cast<int>(renderBufferFrames));

        std::vector<float> inputLeft(renderBufferFrames, 0.0f);
        std::vector<float> inputRight(renderBufferFrames, 0.0f);
        std::vector<float> outputLeft(renderBufferFrames, 0.0f);
        std::vector<float> outputRight(renderBufferFrames, 0.0f);
        const auto captureBufferFrames =
            std::max<std::size_t>(4096, renderBufferFrames * 8);
        const auto captureTargetFillFrames = std::min(
            captureBufferFrames / 2,
            std::max<std::size_t>(512, renderBufferFrames * 2));
        StereoRingBuffer capturedAudio(
            captureBufferFrames,
            captureTargetFillFrames);

        BYTE* initialOutput = nullptr;
        throwIfFailed(
            renderClient->GetBuffer(renderBufferFrames, &initialOutput),
            "Priming the WASAPI output buffer");
        throwIfFailed(
            renderClient->ReleaseBuffer(
                renderBufferFrames, AUDCLNT_BUFFERFLAGS_SILENT),
            "Releasing the primed WASAPI output buffer");

        if (captureAvailable)
        {
            const HRESULT captureStartResult = captureAudioClient->Start();
            if (FAILED(captureStartResult))
            {
                captureAvailable = false;
                captureClient.Reset();
                captureAudioClient.Reset();
                std::cerr << "WASAPI capture failed to start; effect input will be silent"
                          << std::endl;
            }
        }

        throwIfFailed(renderAudioClient->Start(), "Starting the WASAPI output");

        startupPromise.set_value();
        startupReported = true;
        std::println(
            "WASAPI shared-mode audio started ({}, stereo, {} Hz, buffer: {} frames, period: {} frames)",
            renderUsesAudioClient3 ? "IAudioClient3" : "IAudioClient",
            sampleRate,
            renderBufferFrames,
            renderUsesAudioClient3 ? renderPeriodFrames : 0);
        if (captureAvailable)
        {
            std::println(
                "WASAPI capture started ({}, period: {} frames)",
                captureUsesAudioClient3 ? "IAudioClient3" : "IAudioClient",
                captureUsesAudioClient3 ? capturePeriodFrames : 0);
        }

        auto disableCapture = [&](const char* operation, HRESULT result)
        {
            if (!captureAvailable)
                return;

            captureAvailable = false;
            if (captureAudioClient)
                captureAudioClient->Stop();
            captureClient.Reset();
            captureAudioClient.Reset();
            capturedAudio.clear();

            std::cerr << "WASAPI capture disabled after " << operation
                      << " failed with HRESULT 0x" << std::hex
                      << static_cast<unsigned long>(result) << std::dec
                      << "; effect input will be silent" << std::endl;
        };

        auto drainCapture = [&]()
        {
            if (!captureAvailable)
                return;

            UINT32 packetFrames = 0;
            HRESULT result = captureClient->GetNextPacketSize(&packetFrames);
            if (FAILED(result))
            {
                disableCapture("getting the capture packet size", result);
                return;
            }

            while (packetFrames > 0)
            {
                BYTE* data = nullptr;
                UINT32 frames = 0;
                DWORD flags = 0;
                result = captureClient->GetBuffer(
                    &data, &frames, &flags, nullptr, nullptr);
                if (FAILED(result))
                {
                    disableCapture("getting a capture buffer", result);
                    return;
                }

                if ((flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY) != 0)
                    capturedAudio.clear();

                const bool silent = (flags & AUDCLNT_BUFFERFLAGS_SILENT) != 0;
                capturedAudio.push(
                    reinterpret_cast<const float*>(data), frames, silent);

                result = captureClient->ReleaseBuffer(frames);
                if (FAILED(result))
                {
                    disableCapture("releasing a capture buffer", result);
                    return;
                }

                result = captureClient->GetNextPacketSize(&packetFrames);
                if (FAILED(result))
                {
                    disableCapture("getting the capture packet size", result);
                    return;
                }
            }
        };

        while (true)
        {
            HANDLE events[] = {
                static_cast<HANDLE>(stopEvent_),
                renderEvent.get(),
                captureEvent.get()
            };
            const DWORD eventCount = captureAvailable ? 3 : 2;
            const DWORD waitResult = WaitForMultipleObjects(
                eventCount, events, FALSE, INFINITE);

            if (waitResult == WAIT_OBJECT_0)
                break;
            if (waitResult == WAIT_FAILED)
                throw std::runtime_error("Waiting for a WASAPI event failed");
            if (captureAvailable && waitResult == WAIT_OBJECT_0 + 2)
            {
                drainCapture();
                continue;
            }
            if (waitResult != WAIT_OBJECT_0 + 1)
                continue;

            drainCapture();

            UINT32 padding = 0;
            throwIfFailed(
                renderAudioClient->GetCurrentPadding(&padding),
                "Getting WASAPI output padding");
            const UINT32 frames = renderBufferFrames - padding;
            if (frames == 0)
                continue;

            BYTE* outputData = nullptr;
            throwIfFailed(
                renderClient->GetBuffer(frames, &outputData),
                "Getting a WASAPI output buffer");

            capturedAudio.pop(inputLeft.data(), inputRight.data(), frames);
            std::fill_n(outputLeft.data(), frames, 0.0f);
            std::fill_n(outputRight.data(), frames, 0.0f);

            this->processParameterChanges();
            this->resetOutputParameters();

            float* outputPointers[] = {outputLeft.data(), outputRight.data()};
            if constexpr (PluginType::isInstrument)
            {
                this->template processInstrument<float>(
                    std::span<float* const>(outputPointers, stereoChannels),
                    static_cast<int>(frames),
                    std::span<const MidiEvent>{});
            }
            else
            {
                const float* inputPointers[] = {inputLeft.data(), inputRight.data()};
                this->template processEffect<float>(
                    std::span<const float* const>(inputPointers, stereoChannels),
                    std::span<float* const>(outputPointers, stereoChannels),
                    static_cast<int>(frames));
            }

            this->publishOutputParameters();

            auto* interleavedOutput = reinterpret_cast<float*>(outputData);
            for (UINT32 frame = 0; frame < frames; ++frame)
            {
                interleavedOutput[frame * stereoChannels] = outputLeft[frame];
                interleavedOutput[frame * stereoChannels + 1] = outputRight[frame];
            }

            throwIfFailed(
                renderClient->ReleaseBuffer(frames, 0),
                "Releasing a WASAPI output buffer");
        }

        if (captureAvailable)
            captureAudioClient->Stop();
        renderAudioClient->Stop();
    }
    catch (...)
    {
        if (!startupReported)
        {
            try
            {
                startupPromise.set_exception(std::current_exception());
            }
            catch (...)
            {
            }
            return;
        }

        try
        {
            throw;
        }
        catch (const std::exception& error)
        {
            std::cerr << "WASAPI audio stopped: " << error.what() << std::endl;
        }
        catch (...)
        {
            std::cerr << "WASAPI audio stopped with an unknown error" << std::endl;
        }
    }
}

template<typename PluginType>
std::vector<AudioDevice> WASAPI<PluginType>::probeDevices() const
{
    ComApartment comApartment;
    if (!comApartment.isAvailable())
        throw std::runtime_error("Failed to initialize COM for WASAPI device enumeration");

    std::vector<AudioDevice> defaultDevices;
    std::vector<AudioDevice> otherDevices;

    ComPtr<IMMDeviceEnumerator> enumerator;
    HRESULT result = CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        nullptr,
        CLSCTX_ALL,
        IID_PPV_ARGS(enumerator.GetAddressOf()));

    if (SUCCEEDED(result))
    {
        std::wstring defaultDeviceId;
        ComPtr<IMMDevice> defaultEndpoint;
        if (SUCCEEDED(enumerator->GetDefaultAudioEndpoint(
                eRender, eConsole, &defaultEndpoint)))
        {
            wchar_t* endpointId = nullptr;
            if (SUCCEEDED(defaultEndpoint->GetId(&endpointId)))
            {
                defaultDeviceId = endpointId;
                CoTaskMemFree(endpointId);
            }
        }

        ComPtr<IMMDeviceCollection> endpoints;
        result = enumerator->EnumAudioEndpoints(
            eRender, DEVICE_STATE_ACTIVE, &endpoints);

        if (SUCCEEDED(result))
        {
            UINT count = 0;
            result = endpoints->GetCount(&count);
            for (UINT index = 0; SUCCEEDED(result) && index < count; ++index)
            {
                ComPtr<IMMDevice> endpoint;
                result = endpoints->Item(index, &endpoint);
                if (FAILED(result))
                    break;

                wchar_t* endpointId = nullptr;
                const bool hasId = SUCCEEDED(endpoint->GetId(&endpointId));
                const bool isDefault = hasId && defaultDeviceId == endpointId;
                if (endpointId != nullptr)
                    CoTaskMemFree(endpointId);

                auto device = describeRenderDevice(*endpoint.Get(), isDefault);
                if (isDefault)
                    defaultDevices.push_back(std::move(device));
                else
                    otherDevices.push_back(std::move(device));
            }
        }
    }

    if (FAILED(result))
        throw std::runtime_error("Failed to enumerate WASAPI output devices");

    defaultDevices.insert(
        defaultDevices.end(),
        std::make_move_iterator(otherDevices.begin()),
        std::make_move_iterator(otherDevices.end()));

    for (std::size_t index = 0; index < defaultDevices.size(); ++index)
        defaultDevices[index].id = static_cast<int>(index);

    return defaultDevices;
}

template class WASAPI<PLUGIN_CLASS>;
