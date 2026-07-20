#pragma once

#include "SingularityPlugin.h"
#include "utilities/SingularityQueue.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <new>
#include <span>
#include <type_traits>

extern "C"
{
#include "capi.h"
}

#ifndef SINGULARITY_CAPI_MAX_BLOCK_SIZE
#define SINGULARITY_CAPI_MAX_BLOCK_SIZE 4096
#endif

#ifndef SINGULARITY_CAPI_STACK_SIZE
#define SINGULARITY_CAPI_STACK_SIZE 4096
#endif

namespace Singularity::Capi
{
namespace Detail
{
template<typename T>
bool readPayload(capi_buf_t& payload, T*& value)
{
    if (payload.data_ptr == nullptr || payload.actual_data_len < sizeof(T))
        return false;
    value = reinterpret_cast<T*>(payload.data_ptr);
    return true;
}

template<typename T>
capi_err_t writePayload(capi_buf_t& payload, const T& value)
{
    if (payload.data_ptr == nullptr)
        return CAPI_EBADPARAM;
    if (payload.max_data_len < sizeof(T))
    {
        payload.actual_data_len = 0;
        return CAPI_ENEEDMORE;
    }

    std::memcpy(payload.data_ptr, &value, sizeof(T));
    payload.actual_data_len = sizeof(T);
    return CAPI_EOK;
}

inline double constrainParameterValue(const Parameter& parameter, double value)
{
    if (parameter.type == ParamType::Choice && !parameter.choices.empty())
        return std::clamp(std::round(value), 0.0,
            static_cast<double>(parameter.choices.size() - 1));
    if (parameter.type == ParamType::Stepped && parameter.steps > 1)
        return std::clamp(std::round(value), 0.0,
            static_cast<double>(parameter.steps - 1));

    value = std::clamp(value, parameter.minValue, parameter.maxValue);
    if (parameter.type == ParamType::Bool)
        return value >= (parameter.minValue + parameter.maxValue) * 0.5
            ? parameter.maxValue : parameter.minValue;
    if (parameter.type == ParamType::Stepped || parameter.type == ParamType::Choice)
        return std::clamp(std::round(value), parameter.minValue, parameter.maxValue);
    return value;
}
} // namespace Detail

template<SingularityPlugin PluginType>
class Adapter
{
public:
    static constexpr auto parameterCount =
        std::tuple_size_v<decltype(PluginType::getParameters())>;

    static capi_err_t getStaticProperties(capi_proplist_t*, capi_proplist_t* properties)
    {
        return getPropertiesImpl(nullptr, properties);
    }

    static capi_err_t init(capi_t* capi, capi_proplist_t* properties)
    {
        if (capi == nullptr ||
            reinterpret_cast<std::uintptr_t>(capi) % alignof(Instance) != 0)
            return CAPI_EBADPARAM;

        auto* instance = new (capi) Instance;
        instance->capi.vtbl_ptr = &vtable_;

        const auto result = setPropertiesImpl(instance, properties, true);
        if (CAPI_FAILED(result))
        {
            instance->~Instance();
            return result;
        }
        return CAPI_EOK;
    }

private:
    struct MediaFormat
    {
        capi_set_get_media_format_t header{};
        capi_standard_data_format_v2_t format{};
        std::array<capi_channel_type_t, CAPI_MAX_CHANNELS_V2> channelTypes{};
    };

    struct Instance
    {
        Instance()
        {
            for (std::size_t i = 0; i < parameters.size(); ++i)
            {
                parameters[i] = {metadata[i].id, metadata[i].defaultValue};
                controlParameters[i] = parameters[i];
                publishedParameters[i] = parameters[i];
            }
        }

        capi_t capi{};
        PluginType plugin{};
        decltype(PluginType::getParameters()) metadata = PluginType::getParameters();
        std::array<ParamList::ParamValue, parameterCount> parameters{};
        std::array<ParamList::ParamValue, parameterCount> controlParameters{};
        std::array<ParamList::ParamValue, parameterCount> publishedParameters{};
        // Requires set/get_param to be serialized on one control thread, giving
        // each queue exactly one producer and one consumer relative to process().
        SingularityQueue<ParameterChange, 256, 8> parameterChanges;
        SingularityQueue<ParameterChange, 256, 8> outputParameterChanges;
        capi_event_callback_info_t eventCallback{};
        MediaFormat mediaFormat{};
        bool hasMediaFormat = false;
    };

    static_assert(alignof(Instance) <= 8,
        "AudioReach only guarantees 8-byte alignment for CAPI instance memory");
    static_assert(std::atomic<std::size_t>::is_always_lock_free,
        "The CAPI parameter queues require lock-free size_t atomics");

    static constexpr std::uint32_t mediaFormatSize(std::uint32_t channels)
    {
        return sizeof(capi_set_get_media_format_t) +
            sizeof(capi_standard_data_format_v2_t) +
            channels * sizeof(capi_channel_type_t);
    }

    static capi_err_t applyMediaFormat(Instance& instance,
                                       capi_prop_t& property,
                                       bool isInput)
    {
        if (isInput == PluginType::isInstrument)
            return CAPI_EUNSUPPORTED;
        if (!property.port_info.is_valid ||
            property.port_info.is_input_port != isInput ||
            property.port_info.port_index != 0)
            return CAPI_EBADPARAM;

        const auto minimumSize = mediaFormatSize(0);
        if (property.payload.data_ptr == nullptr ||
            property.payload.actual_data_len < minimumSize)
            return CAPI_ENEEDMORE;

        const auto* source = reinterpret_cast<const MediaFormat*>(property.payload.data_ptr);
        const auto channels = source->format.num_channels;
        if (source->header.format_header.data_format != CAPI_FLOATING_POINT ||
            source->format.bits_per_sample != 32 ||
            source->format.data_interleaving != CAPI_DEINTERLEAVED_UNPACKED ||
            source->format.data_is_signed == 0 ||
            source->format.sampling_rate == 0 ||
            channels == 0 || channels > CAPI_MAX_CHANNELS_V2)
            return CAPI_EBADPARAM;

        const auto requiredSize = mediaFormatSize(channels);
        if (property.payload.actual_data_len < requiredSize)
            return CAPI_ENEEDMORE;

        std::memset(&instance.mediaFormat, 0, sizeof(instance.mediaFormat));
        std::memcpy(&instance.mediaFormat, source, requiredSize);
        instance.hasMediaFormat = true;
        instance.plugin.prepare(source->format.sampling_rate,
                                SINGULARITY_CAPI_MAX_BLOCK_SIZE);

        return CAPI_EOK;
    }

    static void resetPlugin(Instance& instance)
    {
        if constexpr (requires(PluginType& plugin) { plugin.reset(); })
            instance.plugin.reset();
        else
        {
            std::destroy_at(&instance.plugin);
            std::construct_at(&instance.plugin);
        }

        if (instance.hasMediaFormat)
            instance.plugin.prepare(instance.mediaFormat.format.sampling_rate,
                                    SINGULARITY_CAPI_MAX_BLOCK_SIZE);
    }

    static capi_err_t process(capi_t* capi,
                              capi_stream_data_t* input[],
                              capi_stream_data_t* output[])
    {
        auto* instance = asInstance(capi);
        if (instance == nullptr || !instance->hasMediaFormat)
            return CAPI_ENOTREADY;
        if (output == nullptr || output[0] == nullptr)
            return CAPI_EBADPARAM;

        auto& out = *output[0];
        const auto channels = instance->mediaFormat.format.num_channels;
        if (out.buf_ptr == nullptr || out.bufs_num < channels)
            return CAPI_EBADPARAM;

        capi_stream_data_t* inputStream = nullptr;
        if constexpr (!PluginType::isInstrument)
        {
            if (input == nullptr || input[0] == nullptr)
                return CAPI_EBADPARAM;
            inputStream = input[0];
            if (inputStream->buf_ptr == nullptr || inputStream->bufs_num < channels)
                return CAPI_EBADPARAM;
        }

        std::uint32_t samples = std::numeric_limits<std::uint32_t>::max();
        for (std::uint32_t channel = 0; channel < channels; ++channel)
        {
            if (out.buf_ptr[channel].data_ptr == nullptr)
                return CAPI_EBADPARAM;
            if (out.buf_ptr[channel].max_data_len % sizeof(float) != 0)
                return CAPI_EBADPARAM;
            if constexpr (!PluginType::isInstrument)
            {
                if (inputStream->buf_ptr[channel].data_ptr == nullptr)
                    return CAPI_EBADPARAM;
                if (inputStream->buf_ptr[channel].actual_data_len >
                        inputStream->buf_ptr[channel].max_data_len ||
                    inputStream->buf_ptr[channel].actual_data_len % sizeof(float) != 0)
                    return CAPI_EBADPARAM;
                const auto channelSamples = inputStream->buf_ptr[channel].actual_data_len /
                    static_cast<std::uint32_t>(sizeof(float));
                if (channel == 0)
                    samples = channelSamples;
                else if (channelSamples != samples)
                    return CAPI_EBADPARAM;
            }
            const auto outputCapacity = out.buf_ptr[channel].max_data_len /
                static_cast<std::uint32_t>(sizeof(float));
            if constexpr (PluginType::isInstrument)
                samples = std::min(samples, outputCapacity);
            else if (outputCapacity < samples)
                return CAPI_ENEEDMORE;
        }

        std::array<const float*, CAPI_MAX_CHANNELS_V2> inputChannels{};
        std::array<float*, CAPI_MAX_CHANNELS_V2> outputChannels{};
        for (std::uint32_t channel = 0; channel < channels; ++channel)
        {
            if constexpr (!PluginType::isInstrument)
                inputChannels[channel] = reinterpret_cast<const float*>(
                    inputStream->buf_ptr[channel].data_ptr);
            outputChannels[channel] = reinterpret_cast<float*>(out.buf_ptr[channel].data_ptr);
        }

        ParameterChange change;
        while (instance->parameterChanges.pop(change))
        {
            for (auto& [parameterId, value] : instance->parameters)
            {
                if (parameterId != static_cast<unsigned int>(change.id))
                    continue;
                value = change.value;
                break;
            }
        }

        for (std::size_t parameterIndex = 0;
             parameterIndex < instance->metadata.size(); ++parameterIndex)
            if (instance->metadata[parameterIndex].readOnly)
                instance->parameters[parameterIndex].second =
                    instance->metadata[parameterIndex].defaultValue;

        ParamList params{instance->parameters};
        std::uint32_t processedSamples = 0;
        while (processedSamples < samples)
        {
            const auto blockSamples = std::min<std::uint32_t>(
                samples - processedSamples, SINGULARITY_CAPI_MAX_BLOCK_SIZE);
            std::array<const float*, CAPI_MAX_CHANNELS_V2> blockInputs{};
            std::array<float*, CAPI_MAX_CHANNELS_V2> blockOutputs{};
            for (std::uint32_t channel = 0; channel < channels; ++channel)
            {
                if constexpr (!PluginType::isInstrument)
                    blockInputs[channel] = inputChannels[channel] + processedSamples;
                blockOutputs[channel] = outputChannels[channel] + processedSamples;
            }

            if constexpr (PluginType::isInstrument)
            {
                instance->plugin.template process<float>(
                    std::span<float* const>(blockOutputs.data(), channels),
                    static_cast<int>(blockSamples), std::span<const MidiEvent>{}, params);
            }
            else
            {
                instance->plugin.template process<float>(
                    std::span<const float* const>(blockInputs.data(), channels),
                    std::span<float* const>(blockOutputs.data(), channels),
                    static_cast<int>(blockSamples), params);
            }
            processedSamples += blockSamples;
        }

        for (std::size_t i = 0; i < instance->metadata.size(); ++i)
        {
            if (!instance->metadata[i].readOnly ||
                instance->parameters[i].second == instance->publishedParameters[i].second)
                continue;

            const ParameterChange outputChange{
                static_cast<int>(instance->parameters[i].first),
                instance->parameters[i].second
            };
            if (instance->outputParameterChanges.push(outputChange))
                instance->publishedParameters[i].second = outputChange.value;
        }

        const auto bytes = samples * sizeof(float);
        for (std::uint32_t channel = 0; channel < channels; ++channel)
        {
            if constexpr (!PluginType::isInstrument)
                inputStream->buf_ptr[channel].actual_data_len = bytes;
            out.buf_ptr[channel].actual_data_len = bytes;
        }
        return CAPI_EOK;
    }

    static capi_err_t end(capi_t* capi)
    {
        auto* instance = asInstance(capi);
        if (instance == nullptr)
            return CAPI_EBADPARAM;
        instance->capi.vtbl_ptr = nullptr;
        instance->~Instance();
        return CAPI_EOK;
    }

    static capi_err_t setParameter(capi_t* capi, std::uint32_t id,
                                   const capi_port_info_t*, capi_buf_t* payload)
    {
        auto* instance = asInstance(capi);
        if (instance == nullptr || payload == nullptr || payload->data_ptr == nullptr)
            return CAPI_EBADPARAM;
        if (payload->actual_data_len < sizeof(float))
            return CAPI_ENEEDMORE;

        float value = 0.0f;
        std::memcpy(&value, payload->data_ptr, sizeof(value));
        if (!std::isfinite(value))
            return CAPI_EBADPARAM;
        for (std::size_t i = 0; i < instance->metadata.size(); ++i)
        {
            const auto& parameter = instance->metadata[i];
            if (parameter.id != id)
                continue;
            if (parameter.readOnly)
                return CAPI_EUNSUPPORTED;
            const auto constrainedValue = Detail::constrainParameterValue(parameter, value);
            if (!instance->parameterChanges.push({static_cast<int>(id), constrainedValue}))
                return CAPI_ENOMEMORY;
            instance->controlParameters[i].second = constrainedValue;
            return CAPI_EOK;
        }
        return CAPI_EUNSUPPORTED;
    }

    static capi_err_t getParameter(capi_t* capi, std::uint32_t id,
                                   const capi_port_info_t*, capi_buf_t* payload)
    {
        auto* instance = asInstance(capi);
        if (instance == nullptr || payload == nullptr || payload->data_ptr == nullptr)
            return CAPI_EBADPARAM;
        if (payload->max_data_len < sizeof(float))
        {
            payload->actual_data_len = 0;
            return CAPI_ENEEDMORE;
        }

        ParameterChange change;
        while (instance->outputParameterChanges.pop(change))
        {
            for (auto& [parameterId, value] : instance->controlParameters)
            {
                if (parameterId != static_cast<unsigned int>(change.id))
                    continue;
                value = change.value;
                break;
            }
        }

        for (const auto& [parameterId, value] : instance->controlParameters)
        {
            if (parameterId != id)
                continue;
            const auto result = static_cast<float>(value);
            std::memcpy(payload->data_ptr, &result, sizeof(result));
            payload->actual_data_len = sizeof(result);
            return CAPI_EOK;
        }
        payload->actual_data_len = 0;
        return CAPI_EUNSUPPORTED;
    }

    static capi_err_t setProperties(capi_t* capi, capi_proplist_t* properties)
    {
        return setPropertiesImpl(asInstance(capi), properties, false);
    }

    static capi_err_t getProperties(capi_t* capi, capi_proplist_t* properties)
    {
        return getPropertiesImpl(asInstance(capi), properties);
    }

    static capi_err_t setPropertiesImpl(Instance* instance,
                                        capi_proplist_t* properties,
                                        bool isInitializing)
    {
        if (instance == nullptr)
            return CAPI_EBADPARAM;
        if (properties == nullptr)
            return CAPI_EOK;
        if (properties->props_num > 0 && properties->prop_ptr == nullptr)
            return CAPI_EBADPARAM;

        capi_err_t result = CAPI_EOK;
        bool inputMediaFormatChanged = false;
        for (std::uint32_t i = 0; i < properties->props_num; ++i)
        {
            auto& property = properties->prop_ptr[i];
            switch (property.id)
            {
                case CAPI_EVENT_CALLBACK_INFO:
                {
                    capi_event_callback_info_t* callback = nullptr;
                    if (!Detail::readPayload(property.payload, callback))
                        CAPI_SET_ERROR(result, CAPI_ENEEDMORE);
                    else
                        instance->eventCallback = *callback;
                    break;
                }
                case CAPI_PORT_NUM_INFO:
                {
                    capi_port_num_info_t* ports = nullptr;
                    if (!Detail::readPayload(property.payload, ports))
                        CAPI_SET_ERROR(result, CAPI_ENEEDMORE);
                    else if (ports->num_input_ports != (PluginType::isInstrument ? 0u : 1u) ||
                            ports->num_output_ports != 1)
                        CAPI_SET_ERROR(result, CAPI_EBADPARAM);
                    break;
                }
                case CAPI_INPUT_MEDIA_FORMAT_V2:
                {
                    const auto propertyResult = applyMediaFormat(*instance, property, true);
                    if (propertyResult == CAPI_EOK)
                        inputMediaFormatChanged = true;
                    else if (CAPI_IS_ERROR_CODE_SET(propertyResult, CAPI_EUNSUPPORTED))
                        property.payload.actual_data_len = 0;
                    CAPI_SET_ERROR(result, propertyResult);
                    break;
                }
                case CAPI_OUTPUT_MEDIA_FORMAT_V2:
                {
                    const auto propertyResult = applyMediaFormat(*instance, property, false);
                    if (CAPI_IS_ERROR_CODE_SET(propertyResult, CAPI_EUNSUPPORTED))
                        property.payload.actual_data_len = 0;
                    CAPI_SET_ERROR(result, propertyResult);
                    break;
                }
                case CAPI_ALGORITHMIC_RESET:
                    resetPlugin(*instance);
                    break;
                case CAPI_HEAP_ID:
                {
                    capi_heap_id_t* heapId = nullptr;
                    if (!Detail::readPayload(property.payload, heapId))
                        CAPI_SET_ERROR(result, CAPI_ENEEDMORE);
                    break;
                }
                default:
                    property.payload.actual_data_len = 0;
                    if (!isInitializing)
                        CAPI_SET_ERROR(result, CAPI_EUNSUPPORTED);
                    break;
            }
        }
        if (result == CAPI_EOK && inputMediaFormatChanged)
            CAPI_SET_ERROR(result, raiseOutputMediaFormat(*instance));
        return result;
    }

    static capi_err_t getPropertiesImpl(Instance* instance, capi_proplist_t* properties)
    {
        if (properties == nullptr)
            return CAPI_EOK;
        if (properties->props_num > 0 && properties->prop_ptr == nullptr)
            return CAPI_EBADPARAM;

        capi_err_t result = CAPI_EOK;
        for (std::uint32_t i = 0; i < properties->props_num; ++i)
        {
            auto& property = properties->prop_ptr[i];
            capi_err_t propertyResult = CAPI_EOK;
            switch (property.id)
            {
                case CAPI_INIT_MEMORY_REQUIREMENT:
                    propertyResult = Detail::writePayload(property.payload,
                        capi_init_memory_requirement_t{static_cast<std::uint32_t>((sizeof(Instance) + 7u) & ~7u)});
                    break;
                case CAPI_STACK_SIZE:
                    propertyResult = Detail::writePayload(property.payload,
                        capi_stack_size_t{SINGULARITY_CAPI_STACK_SIZE});
                    break;
                case CAPI_IS_INPLACE:
                    propertyResult = Detail::writePayload(property.payload, capi_is_inplace_t{FALSE});
                    break;
                case CAPI_REQUIRES_DATA_BUFFERING:
                    propertyResult = Detail::writePayload(property.payload,
                        capi_requires_data_buffering_t{FALSE});
                    break;
                case CAPI_MAX_METADATA_SIZE:
                    propertyResult = Detail::writePayload(property.payload,
                        capi_max_metadata_size_t{0, 0});
                    break;
                case CAPI_NUM_NEEDED_FRAMEWORK_EXTENSIONS:
                    propertyResult = Detail::writePayload(property.payload,
                        capi_num_needed_framework_extensions_t{0});
                    break;
                case CAPI_IS_ELEMENTARY:
                    propertyResult = Detail::writePayload(property.payload,
                        capi_is_elementary_t{FALSE});
                    break;
                case CAPI_MIN_PORT_NUM_INFO:
                    propertyResult = Detail::writePayload(property.payload,
                        capi_min_port_num_info_t{PluginType::isInstrument ? 0u : 1u, 1});
                    break;
                case CAPI_OUTPUT_MEDIA_FORMAT_SIZE:
                    propertyResult = Detail::writePayload(property.payload,
                        capi_output_media_format_size_t{
                            static_cast<std::uint32_t>(sizeof(capi_standard_data_format_v2_t) +
                                CAPI_MAX_CHANNELS_V2 * sizeof(capi_channel_type_t))});
                    break;
                case CAPI_INTERFACE_EXTENSIONS:
                {
                    if (property.payload.data_ptr == nullptr ||
                        property.payload.max_data_len < sizeof(capi_interface_extns_list_t))
                    {
                        propertyResult = CAPI_ENEEDMORE;
                        break;
                    }
                    auto* list = reinterpret_cast<capi_interface_extns_list_t*>(property.payload.data_ptr);
                    const auto required = sizeof(capi_interface_extns_list_t) +
                        list->num_extensions * sizeof(capi_interface_extn_desc_t);
                    if (property.payload.max_data_len < required)
                    {
                        propertyResult = CAPI_ENEEDMORE;
                        break;
                    }
                    auto* extension = reinterpret_cast<capi_interface_extn_desc_t*>(
                        property.payload.data_ptr + sizeof(capi_interface_extns_list_t));
                    for (std::uint32_t extensionIndex = 0;
                         extensionIndex < list->num_extensions; ++extensionIndex)
                        extension[extensionIndex].is_supported = FALSE;
                    property.payload.actual_data_len = required;
                    break;
                }
                case CAPI_OUTPUT_MEDIA_FORMAT_V2:
                    if (instance == nullptr || !instance->hasMediaFormat)
                        propertyResult = CAPI_ENOTREADY;
                    else if (!property.port_info.is_valid ||
                             property.port_info.is_input_port ||
                             property.port_info.port_index != 0)
                        propertyResult = CAPI_EBADPARAM;
                    else
                    {
                        const auto size = mediaFormatSize(instance->mediaFormat.format.num_channels);
                        if (property.payload.data_ptr == nullptr)
                            propertyResult = CAPI_EBADPARAM;
                        else if (property.payload.max_data_len < size)
                            propertyResult = CAPI_ENEEDMORE;
                        else
                        {
                            std::memcpy(property.payload.data_ptr, &instance->mediaFormat, size);
                            property.payload.actual_data_len = size;
                        }
                    }
                    break;
                default:
                    property.payload.actual_data_len = 0;
                    propertyResult = CAPI_EUNSUPPORTED;
                    break;
            }
            CAPI_SET_ERROR(result, propertyResult);
        }
        return result;
    }

    static capi_err_t raiseOutputMediaFormat(Instance& instance)
    {
        if (instance.eventCallback.event_cb == nullptr)
            return CAPI_EOK;

        capi_event_info_t event{};
        event.port_info.is_valid = TRUE;
        event.port_info.is_input_port = FALSE;
        event.port_info.port_index = 0;
        event.payload.data_ptr = reinterpret_cast<int8_t*>(&instance.mediaFormat);
        event.payload.actual_data_len = mediaFormatSize(instance.mediaFormat.format.num_channels);
        event.payload.max_data_len = sizeof(instance.mediaFormat);
        return instance.eventCallback.event_cb(instance.eventCallback.event_context,
            CAPI_EVENT_OUTPUT_MEDIA_FORMAT_UPDATED_V2, &event);
    }

    static Instance* asInstance(capi_t* capi)
    {
        return reinterpret_cast<Instance*>(capi);
    }

    inline static const capi_vtbl_t vtable_{
        process,
        end,
        setParameter,
        getParameter,
        setProperties,
        getProperties,
    };
};
} // namespace Singularity::Capi
