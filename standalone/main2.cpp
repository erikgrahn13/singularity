#include <QDebug>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QGuiApplication>
#include <QQmlEngine>
#include <QTimer>
#include <QUrl>
#include <QQuickView>

#include <algorithm>
#include <array>
#include <iostream>

#include "../SingularityController.h"
#include "APPController.h"
#include "RtAudio.h"

#include PLUGIN_CLASS_HEADER

struct AudioContext
{
    PLUGIN_CLASS plugin;
    SingularityQueue<ParameterChange, 256> parameterChanges;
    std::vector<std::pair<unsigned int, double>> parameters;
    unsigned int inputChannels = 0;
    unsigned int outputChannels = 0;
};

template <typename PluginType>
requires (PluginType::isInstrument)
void processPluginAudio(
    PluginType& plugin,
    std::span<const float* const>,
    std::span<float* const> outputs,
    unsigned int frameCount,
    ParamList params)
{
    plugin.template process<float>(
        outputs,
        static_cast<int>(frameCount),
        std::span<const MidiEvent>{},
        params);
}

template <typename PluginType>
requires (!PluginType::isInstrument)
void processPluginAudio(
    PluginType& plugin,
    std::span<const float* const> inputs,
    std::span<float* const> outputs,
    unsigned int frameCount,
    ParamList params)
{
    plugin.template process<float>(
        inputs,
        outputs,
        static_cast<int>(frameCount),
        params);
}

int audioCallback( void *outputBuffer, void *inputBuffer, unsigned int frameCount,
           double streamTime, RtAudioStreamStatus status, void *userData )
{
    auto& context = *static_cast<AudioContext*>(userData);

    ParameterChange change;
    while (context.parameterChanges.pop(change))
    {
        for (auto& [id, value] : context.parameters)
        {
            if (id == static_cast<unsigned int>(change.id))
            {
                value = change.value;
                break;
            }
        }
    }

    auto* input = static_cast<float*>(inputBuffer);
    auto* output = static_cast<float*>(outputBuffer);

    std::array<const float*, 2> inputs {};
    std::array<float*, 2> outputs {};

    for (unsigned int channel = 0;
         channel < context.inputChannels;
         ++channel)
        inputs[channel] = input + channel * frameCount;

    for (unsigned int channel = 0;
         channel < context.outputChannels;
         ++channel)
        outputs[channel] = output + channel * frameCount;

    processPluginAudio(
        context.plugin,
        std::span<const float* const>(inputs.data(), context.inputChannels),
        std::span<float* const>(outputs.data(), context.outputChannels),
        frameCount,
        ParamList {context.parameters});

    return 0;
}


int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    const auto definitions = PLUGIN_CLASS::getParameters();

    AudioContext audioContext;

    for (const auto& definition : definitions)
    {
        audioContext.parameters.push_back({
            definition.id,
            definition.defaultValue
        });
    }

    RtAudio rtaudio;

    constexpr unsigned int requestedChannels = 2;
    unsigned int bufferFrames = 512;

    RtAudio::StreamParameters inputParameters;
    inputParameters.deviceId = rtaudio.getDefaultInputDevice();
    inputParameters.firstChannel = 0;

    RtAudio::StreamParameters outputParameters;
    outputParameters.deviceId = rtaudio.getDefaultOutputDevice();
    outputParameters.firstChannel = 0;

    const auto inputInfo = rtaudio.getDeviceInfo(inputParameters.deviceId);
    const auto outputInfo = rtaudio.getDeviceInfo(outputParameters.deviceId);

    unsigned int inputChannels = std::min(requestedChannels, inputInfo.inputChannels);
    unsigned int outputChannels = std::min(requestedChannels, outputInfo.outputChannels);

    if (inputParameters.deviceId == outputParameters.deviceId && inputInfo.duplexChannels > 0)
    {
        inputChannels = std::min(inputChannels, inputInfo.duplexChannels);
        outputChannels = std::min(outputChannels, inputInfo.duplexChannels);
    }

    if (inputChannels == 0 || outputChannels == 0)
    {
        std::cerr << "The selected devices cannot provide duplex audio\n";
        return 1;
    }

    const auto supportsSampleRate = [](const RtAudio::DeviceInfo& info, unsigned int rate)
    {
        return rate > 0 &&
            (rate == info.currentSampleRate ||
             rate == info.preferredSampleRate ||
             std::find(info.sampleRates.begin(), info.sampleRates.end(), rate) !=
                 info.sampleRates.end());
    };

    const std::array sampleRateCandidates {
        inputInfo.currentSampleRate,
        outputInfo.currentSampleRate,
        inputInfo.preferredSampleRate,
        outputInfo.preferredSampleRate,
        48000u,
        44100u
    };

    unsigned int sampleRate = 0;
    for (const auto candidate : sampleRateCandidates)
    {
        if (supportsSampleRate(inputInfo, candidate) &&
            supportsSampleRate(outputInfo, candidate))
        {
            sampleRate = candidate;
            break;
        }
    }

    if (sampleRate == 0)
    {
        std::cerr << "The selected input and output devices have no common "
                     "sample rate\n";
        return 1;
    }

    inputParameters.nChannels = inputChannels;
    outputParameters.nChannels = outputChannels;
    audioContext.inputChannels = inputChannels;
    audioContext.outputChannels = outputChannels;

    RtAudio::StreamOptions options;
    options.flags |= RTAUDIO_NONINTERLEAVED;

    const auto error = rtaudio.openStream(&outputParameters, &inputParameters, RTAUDIO_FLOAT32, sampleRate, &bufferFrames, &audioCallback, &audioContext, &options);

    if (error != RTAUDIO_NO_ERROR)
        return 1;

    audioContext.plugin.prepare(sampleRate,static_cast<int>(bufferFrames));

    if (rtaudio.startStream() != RTAUDIO_NO_ERROR)
        return 1;

    APPController appController(definitions, [&audioContext](int id, double value) {
        audioContext.parameterChanges.push({id, value});
    });

    SingularityController controller(appController);


    // auto controller = std::make_unique<SingularityController>(getParameterContainer(), definitions);
    controller.setLogger([](const std::string& msg) {
        std::cout << msg << std::endl;
    });

    QQuickView view;
    view.setResizeMode(QQuickView::SizeViewToRootObject);
    controller.attachToView(view);
    view.show();
    view.requestActivate();

    return QGuiApplication::exec();
}
