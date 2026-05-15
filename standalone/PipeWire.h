#pragma once

#include "ISingularityAudio.h"
#include <pipewire/pipewire.h>

class PipeWire : public ISingularityAudio 
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
    };

    
    static void on_process(void *userdata, struct spa_io_position *position);
    private:
    PipeWire::data data_;



};