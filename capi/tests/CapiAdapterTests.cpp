#include "SingularityCapi.h"
#include "ExampleEffect.h"
#include "ExampleInstrument.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <vector>

class StatefulEffect
{
public:
    static constexpr bool isInstrument = false;

    static auto getParameters()
    {
        return std::array<Parameter, 0>{};
    }

    void prepare(double, int) {}

    template<typename SampleType>
    void process(std::span<const SampleType* const>,
                 std::span<SampleType* const> outputs,
                 int samples,
                 ParamList)
    {
        for (auto* output : outputs)
            for (int sample = 0; sample < samples; ++sample)
                output[sample] = static_cast<SampleType>(processCount_);
        ++processCount_;
    }

private:
    int processCount_ = 0;
};

static_assert(SingularityPlugin<StatefulEffect>);

namespace
{
template<typename PluginType>
using Adapter = Singularity::Capi::Adapter<PluginType>;

struct MediaFormat
{
    capi_set_get_media_format_t header{};
    capi_standard_data_format_v2_t format{};
    std::array<capi_channel_type_t, 2> channelTypes{};
};

[[noreturn]] void fail(const char* message)
{
    std::fprintf(stderr, "CAPI adapter test failed: %s\n", message);
    std::exit(EXIT_FAILURE);
}

void require(bool condition, const char* message)
{
    if (!condition)
        fail(message);
}

MediaFormat makeMediaFormat(std::uint32_t channels = 1)
{
    MediaFormat result{};
    result.header.format_header.data_format = CAPI_FLOATING_POINT;
    result.format.bits_per_sample = 32;
    result.format.bitstream_format = CAPI_DATA_FORMAT_INVALID_VAL;
    result.format.data_interleaving = CAPI_DEINTERLEAVED_UNPACKED;
    result.format.data_is_signed = TRUE;
    result.format.num_channels = channels;
    result.format.q_factor = CAPI_DATA_FORMAT_INVALID_VAL;
    result.format.sampling_rate = 48000;
    result.channelTypes[0] = 1;
    result.channelTypes[1] = 2;
    return result;
}

std::uint32_t mediaFormatSize(std::uint32_t channels)
{
    return sizeof(capi_set_get_media_format_t) +
        sizeof(capi_standard_data_format_v2_t) +
        channels * sizeof(capi_channel_type_t);
}

struct EventState
{
    int outputMediaFormatEvents = 0;
};

capi_err_t handleEvent(void* context, capi_event_id_t id, capi_event_info_t*)
{
    auto& state = *static_cast<EventState*>(context);
    if (id == CAPI_EVENT_OUTPUT_MEDIA_FORMAT_UPDATED_V2)
        ++state.outputMediaFormatEvents;
    return CAPI_EOK;
}

template<typename PluginType>
class Instance
{
public:
    Instance(std::uint32_t inputs, std::uint32_t outputs)
    {
        capi_init_memory_requirement_t requirement{};
        capi_is_elementary_t elementary{TRUE};
        std::array<capi_prop_t, 2> staticProperties{};
        staticProperties[0].id = CAPI_INIT_MEMORY_REQUIREMENT;
        staticProperties[0].payload = {
            reinterpret_cast<int8_t*>(&requirement), 0, sizeof(requirement)};
        staticProperties[1].id = CAPI_IS_ELEMENTARY;
        staticProperties[1].payload = {
            reinterpret_cast<int8_t*>(&elementary), 0, sizeof(elementary)};
        capi_proplist_t staticList{
            static_cast<std::uint32_t>(staticProperties.size()), staticProperties.data()};
        require(Adapter<PluginType>::getStaticProperties(nullptr, &staticList) == CAPI_EOK,
                "static property query failed");
        require(requirement.size_in_bytes > sizeof(capi_t), "invalid instance size");
        require(elementary.is_elementary == FALSE, "module was advertised as elementary");

        storage_.resize(requirement.size_in_bytes + 16);
        auto address = reinterpret_cast<std::uintptr_t>(storage_.data());
        address = (address + 7u) & ~std::uintptr_t{7u};
        if ((address & 15u) == 0)
            address += 8;
        capi_ = reinterpret_cast<capi_t*>(address);

        capi_port_num_info_t ports{inputs, outputs};
        capi_event_callback_info_t callback{handleEvent, &events};
        std::array<capi_prop_t, 2> initProperties{};
        initProperties[0].id = CAPI_EVENT_CALLBACK_INFO;
        initProperties[0].payload = {
            reinterpret_cast<int8_t*>(&callback), sizeof(callback), sizeof(callback)};
        initProperties[1].id = CAPI_PORT_NUM_INFO;
        initProperties[1].payload = {
            reinterpret_cast<int8_t*>(&ports), sizeof(ports), sizeof(ports)};
        capi_proplist_t initList{
            static_cast<std::uint32_t>(initProperties.size()), initProperties.data()};
        require(Adapter<PluginType>::init(capi_, &initList) == CAPI_EOK, "init failed");
    }

    ~Instance()
    {
        if (capi_ != nullptr)
            require(capi_->vtbl_ptr->end(capi_) == CAPI_EOK, "end failed");
    }

    capi_t* get() const { return capi_; }

    EventState events;

private:
    std::vector<std::byte> storage_;
    capi_t* capi_ = nullptr;
};

void setMediaFormat(capi_t* capi, MediaFormat& format, bool isInput)
{
    capi_prop_t property{};
    property.id = isInput ? CAPI_INPUT_MEDIA_FORMAT_V2 : CAPI_OUTPUT_MEDIA_FORMAT_V2;
    property.payload = {reinterpret_cast<int8_t*>(&format),
                        mediaFormatSize(format.format.num_channels),
                        sizeof(format)};
    property.port_info = {TRUE, static_cast<bool_t>(isInput ? TRUE : FALSE), 0};
    capi_proplist_t list{1, &property};
    require(capi->vtbl_ptr->set_properties(capi, &list) == CAPI_EOK,
            "media format was rejected");
}

void setParameter(capi_t* capi, std::uint32_t id, float value)
{
    capi_buf_t payload{reinterpret_cast<int8_t*>(&value), sizeof(value), sizeof(value)};
    require(capi->vtbl_ptr->set_param(capi, id, nullptr, &payload) == CAPI_EOK,
            "set_param failed");
}

float getParameter(capi_t* capi, std::uint32_t id)
{
    float value = 0.0f;
    capi_buf_t payload{reinterpret_cast<int8_t*>(&value), 0, sizeof(value)};
    require(capi->vtbl_ptr->get_param(capi, id, nullptr, &payload) == CAPI_EOK,
            "get_param failed");
    require(payload.actual_data_len == sizeof(value), "get_param returned wrong size");
    return value;
}

void verifyCommonRuntimeProperties(capi_t* capi)
{
    capi_port_data_threshold_t threshold{};
    capi_prop_t thresholdProperty{};
    thresholdProperty.id = CAPI_PORT_DATA_THRESHOLD;
    thresholdProperty.payload = {
        reinterpret_cast<int8_t*>(&threshold), 0, sizeof(threshold)};
    capi_proplist_t getList{1, &thresholdProperty};
    const auto thresholdResult = capi->vtbl_ptr->get_properties(capi, &getList);
    require(CAPI_IS_ERROR_CODE_SET(thresholdResult, CAPI_EUNSUPPORTED),
            "unexpected port threshold support");
    require(thresholdProperty.payload.actual_data_len == 0,
            "unsupported threshold returned a payload");

    capi_prop_t unknown{};
    unknown.id = CAPI_CUSTOM_PROPERTY;
    capi_proplist_t setList{1, &unknown};
    const auto unknownResult = capi->vtbl_ptr->set_properties(capi, &setList);
    require(CAPI_IS_ERROR_CODE_SET(unknownResult, CAPI_EUNSUPPORTED),
            "unsupported runtime property was accepted");

    capi_prop_t reset{};
    reset.id = CAPI_ALGORITHMIC_RESET;
    capi_proplist_t resetList{1, &reset};
    require(capi->vtbl_ptr->set_properties(capi, &resetList) == CAPI_EOK,
            "algorithmic reset failed");
}

void testEffect()
{
    Instance<ExampleEffect> instance{1, 1};
    auto format = makeMediaFormat();
    setMediaFormat(instance.get(), format, true);
    require(instance.events.outputMediaFormatEvents == 1,
            "effect did not raise its output media format");

    setParameter(instance.get(), 13, 1.0f);
    setParameter(instance.get(), 14, 2.0f);
    require(getParameter(instance.get(), 14) == 2.0f,
            "choice parameter did not accept its last index");

    std::array<float, 40> input{};
    std::array<float, 40> output{};
    input.fill(0.25f);
    capi_buf_t inputBuffer{reinterpret_cast<int8_t*>(input.data()),
                           sizeof(input), sizeof(input)};
    capi_buf_t outputBuffer{reinterpret_cast<int8_t*>(output.data()),
                            0, sizeof(output)};
    capi_stream_data_t inputStream{};
    inputStream.bufs_num = 1;
    inputStream.buf_ptr = &inputBuffer;
    capi_stream_data_t outputStream{};
    outputStream.bufs_num = 1;
    outputStream.buf_ptr = &outputBuffer;
    capi_stream_data_t* inputs[]{&inputStream};
    capi_stream_data_t* outputs[]{&outputStream};
    require(instance.get()->vtbl_ptr->process(instance.get(), inputs, outputs) == CAPI_EOK,
            "effect process failed");
    require(outputBuffer.actual_data_len == sizeof(output), "effect returned wrong length");
    for (const auto sample : output)
        require(std::abs(sample - 0.25f) < 0.0001f, "effect returned wrong audio");
    require(std::abs(getParameter(instance.get(), 15) - 0.25f) < 0.0001f,
            "read-only parameter was not published");

    verifyCommonRuntimeProperties(instance.get());
}

void testInstrument()
{
    Instance<ExampleInstrument> instance{0, 1};
    auto format = makeMediaFormat();
    setMediaFormat(instance.get(), format, false);

    std::array<float, 40> output{};
    output.fill(1.0f);
    capi_buf_t outputBuffer{reinterpret_cast<int8_t*>(output.data()),
                            0, sizeof(output)};
    capi_stream_data_t outputStream{};
    outputStream.bufs_num = 1;
    outputStream.buf_ptr = &outputBuffer;
    capi_stream_data_t* outputs[]{&outputStream};
    require(instance.get()->vtbl_ptr->process(instance.get(), nullptr, outputs) == CAPI_EOK,
            "instrument process failed");
    require(outputBuffer.actual_data_len == sizeof(output),
            "instrument returned wrong length");
    for (const auto sample : output)
        require(sample == 0.0f, "instrument did not initialize its output");

    verifyCommonRuntimeProperties(instance.get());
}

void testAlgorithmicReset()
{
    Instance<StatefulEffect> instance{1, 1};
    auto format = makeMediaFormat();
    setMediaFormat(instance.get(), format, true);

    float input = 0.0f;
    float output = 0.0f;
    capi_buf_t inputBuffer{reinterpret_cast<int8_t*>(&input), sizeof(input), sizeof(input)};
    capi_buf_t outputBuffer{reinterpret_cast<int8_t*>(&output), 0, sizeof(output)};
    capi_stream_data_t inputStream{};
    inputStream.bufs_num = 1;
    inputStream.buf_ptr = &inputBuffer;
    capi_stream_data_t outputStream{};
    outputStream.bufs_num = 1;
    outputStream.buf_ptr = &outputBuffer;
    capi_stream_data_t* inputs[]{&inputStream};
    capi_stream_data_t* outputs[]{&outputStream};

    require(instance.get()->vtbl_ptr->process(instance.get(), inputs, outputs) == CAPI_EOK,
            "stateful process failed");
    require(output == 0.0f, "stateful effect began in the wrong state");
    outputBuffer.actual_data_len = 0;
    require(instance.get()->vtbl_ptr->process(instance.get(), inputs, outputs) == CAPI_EOK,
            "second stateful process failed");
    require(output == 1.0f, "stateful effect did not advance");

    capi_prop_t reset{};
    reset.id = CAPI_ALGORITHMIC_RESET;
    capi_proplist_t resetList{1, &reset};
    require(instance.get()->vtbl_ptr->set_properties(instance.get(), &resetList) == CAPI_EOK,
            "stateful reset failed");
    outputBuffer.actual_data_len = 0;
    require(instance.get()->vtbl_ptr->process(instance.get(), inputs, outputs) == CAPI_EOK,
            "process after reset failed");
    require(output == 0.0f, "algorithm state was not reset");
}
} // namespace

int main()
{
    testEffect();
    testInstrument();
    testAlgorithmicReset();
    return EXIT_SUCCESS;
}
