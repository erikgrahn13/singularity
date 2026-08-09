#pragma once

#include "ISingularityAudio.h"
#include <pipewire/pipewire.h>

template<typename PluginType>
class PipeWire : public ISingularityAudio<PluginType>
{
    public:
    PipeWire();

    ~PipeWire();

    std::vector<AudioDevice> probeDevices() const override;

    struct port {
        struct data *data;
    };

    struct data {
            struct pw_thread_loop *loop;
            struct pw_filter *filter;
            struct port *in_port;
            struct port *out_port;
            struct spa_io_position *position;
    };

    static void on_state_changed(void *userdata, enum pw_filter_state old,
                                 enum pw_filter_state state, const char *error);
    static void on_io_changed(void *userdata, void *port_data,
                              uint32_t id, void *area, uint32_t size);
    static void on_process(void *userdata, struct spa_io_position *position);
    private:
    void prepareFromPosition();
    PipeWire::data data_{};



};
