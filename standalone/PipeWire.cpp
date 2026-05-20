#include "PipeWire.h"
#include PLUGIN_CLASS_HEADER

#include <iostream>
#include <span>
#include <spa/pod/builder.h>
#include <spa/param/latency-utils.h>
 
#include <pipewire/filter.h>

 
/* [on_process] */
template<typename PluginType>
void PipeWire<PluginType>::on_process(void *userdata, struct spa_io_position *position)
{
        PipeWire<PluginType> *instance = static_cast<PipeWire<PluginType>*>(userdata);
        uint32_t n_samples = position->clock.duration;
        double sample_rate = static_cast<double>(position->clock.rate.denom) / position->clock.rate.num;
        instance->callPrepare(sample_rate, static_cast<int>(n_samples));

        pw_log_trace("do process %d", n_samples);

        float *in  = static_cast<float*>(pw_filter_get_dsp_buffer(instance->data_.in_port,  n_samples));
        float *out = static_cast<float*>(pw_filter_get_dsp_buffer(instance->data_.out_port, n_samples));

        if (in == NULL || out == NULL)
                return;

        // Drain parameter changes queued from the GUI thread
        instance->processParameterChanges();

        // Wrap raw pointers in span arrays matching the plugin interface
        const float* inputPtrs[1]  = { in  };
        float*       outputPtrs[1] = { out };

        instance->mPlugin.template process<float>(
                std::span<const float* const>(inputPtrs,  1),
                std::span<float* const>(outputPtrs, 1),
                n_samples,
                ParamList{instance->_params});
}

// static void do_quit(void *userdata, int signal_number)
// {
//         PipeWire::data *data = static_cast<PipeWire::data*>(userdata);
//         pw_thread_loop_stop(data->loop);
//         pw_thread_loop_destroy(data->loop);
// }

template<typename PluginType>
PipeWire<PluginType>::PipeWire() : ISingularityAudio<PluginType>()
{
        // struct data data = { 0, };
        const struct spa_pod *params[1];
        uint32_t n_params = 0;
        uint8_t buffer[1024];
        struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

        static const struct pw_filter_events filter_events = {
                .version = PW_VERSION_FILTER_EVENTS,
                .process = PipeWire<PluginType>::on_process,
        };
 
        pw_init(nullptr, nullptr);
 
        /* make a main loop. If you already have another main loop, you can add
         * the fd of this pipewire mainloop to it. */
        data_.loop = pw_thread_loop_new("audio", nullptr);
 
        // pw_loop_add_signal(pw_main_loop_get_loop(data_.loop), SIGINT, do_quit, &data_);
        // pw_loop_add_signal(pw_main_loop_get_loop(data_.loop), SIGTERM, do_quit, &data_);
 
        /* Create a simple filter, the simple filter manages the core and remote
         * objects for you if you don't need to deal with them.
         *
         * Pass your events and a user_data pointer as the last arguments. This
         * will inform you about the filter state. The most important event
         * you need to listen to is the process event where you need to process
         * the data.
         */
        data_.filter = pw_filter_new_simple(
                        pw_thread_loop_get_loop(data_.loop),
                        "",
                        pw_properties_new(
                                PW_KEY_MEDIA_TYPE, "Audio",
                                PW_KEY_MEDIA_CATEGORY, "Filter",
                                PW_KEY_MEDIA_ROLE, "DSP",
                                PW_KEY_NODE_NAME, pw_get_prgname(),
                                NULL),
                        &filter_events,
                        this);
 
        /* make an audio DSP input port */
        data_.in_port = static_cast<typename PipeWire<PluginType>::port*>(pw_filter_add_port(data_.filter,
                        PW_DIRECTION_INPUT,
                        PW_FILTER_PORT_FLAG_MAP_BUFFERS,
                        sizeof(struct port),
                        pw_properties_new(
                                PW_KEY_FORMAT_DSP, "32 bit float mono audio",
                                PW_KEY_PORT_NAME, "input",
                                NULL),
                        NULL, 0));
 
        /* make an audio DSP output port */
        data_.out_port = static_cast<typename PipeWire<PluginType>::port*>(pw_filter_add_port(data_.filter,
                        PW_DIRECTION_OUTPUT,
                        PW_FILTER_PORT_FLAG_MAP_BUFFERS,
                        sizeof(struct port),
                        pw_properties_new(
                                PW_KEY_FORMAT_DSP, "32 bit float mono audio",
                                PW_KEY_PORT_NAME, "output",
                                NULL),
                        NULL, 0));
 
        /* Set processing latency information */
        struct spa_process_latency_info latency_info = SPA_PROCESS_LATENCY_INFO_INIT(
            .ns = 10 * SPA_NSEC_PER_MSEC
        );
        params[n_params++] = spa_process_latency_build(&b,
                        SPA_PARAM_ProcessLatency,
                        &latency_info);
 
        /* Now connect this filter. We ask that our process function is
         * called in a realtime thread. */
        if (pw_filter_connect(data_.filter,
                                PW_FILTER_FLAG_RT_PROCESS,
                                params, n_params) < 0) {
                fprintf(stderr, "can't connect\n");
        }
 
        /* and wait while we let things run */
        pw_thread_loop_start(data_.loop);
}

template<typename PluginType>
PipeWire<PluginType>::~PipeWire()
{
        pw_filter_destroy(data_.filter);
        pw_thread_loop_stop(data_.loop);
        pw_thread_loop_destroy(data_.loop);
        pw_deinit();
}

template<typename PluginType>
std::vector<AudioDevice> PipeWire<PluginType>::probeDevices() const
{
    return {};
}

template class PipeWire<PLUGIN_CLASS>;