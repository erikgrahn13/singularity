#pragma once

#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include <linux/input-event-codes.h>
#include <cstring>
#include <functional>
#include <stdexcept>
#include <string>
#include "../IWindow.h"

class WaylandWindow : public IWindow {
public:
    WaylandWindow(const std::string& title, int width, int height)
        : m_width(width), m_height(height)
    {
        m_display = wl_display_connect(nullptr);
        if (!m_display)
            throw std::runtime_error("Failed to connect to Wayland display");

        m_registry = wl_display_get_registry(m_display);

        static const wl_registry_listener registryListener = {
            .global = [](void* data, wl_registry* reg, uint32_t name,
                         const char* iface, uint32_t /*ver*/) {
                auto* win = static_cast<WaylandWindow*>(data);
                if (strcmp(iface, wl_compositor_interface.name) == 0) {
                    win->m_compositor = static_cast<wl_compositor*>(
                        wl_registry_bind(reg, name, &wl_compositor_interface, 1));
                } else if (strcmp(iface, xdg_wm_base_interface.name) == 0) {
                    win->m_wmBase = static_cast<xdg_wm_base*>(
                        wl_registry_bind(reg, name, &xdg_wm_base_interface, 1));
                    static const xdg_wm_base_listener wmBaseListener = {
                        .ping = [](void*, xdg_wm_base* base, uint32_t serial) {
                            xdg_wm_base_pong(base, serial);
                        }
                    };
                    xdg_wm_base_add_listener(win->m_wmBase, &wmBaseListener, nullptr);
                } else if (strcmp(iface, wl_seat_interface.name) == 0) {
                    win->m_seat = static_cast<wl_seat*>(
                        wl_registry_bind(reg, name, &wl_seat_interface, 1));
                }
            },
            .global_remove = [](void*, wl_registry*, uint32_t) {}
        };
        wl_registry_add_listener(m_registry, &registryListener, this);
        wl_display_roundtrip(m_display); // collect globals

        if (!m_compositor) throw std::runtime_error("No wl_compositor");
        if (!m_wmBase)     throw std::runtime_error("No xdg_wm_base");

        if (m_seat) {
            static const wl_seat_listener seatListener = {
                .capabilities = [](void* data, wl_seat* seat, uint32_t caps) {
                    auto* win = static_cast<WaylandWindow*>(data);
                    if ((caps & WL_SEAT_CAPABILITY_POINTER) && !win->m_pointer) {
                        win->m_pointer = wl_seat_get_pointer(seat);
                        static const wl_pointer_listener ptrListener = {
                            .enter  = [](void*, wl_pointer*, uint32_t, wl_surface*,
                                         wl_fixed_t, wl_fixed_t) {},
                            .leave  = [](void*, wl_pointer*, uint32_t, wl_surface*) {},
                            .motion = [](void* d, wl_pointer*, uint32_t,
                                         wl_fixed_t sx, wl_fixed_t sy) {
                                auto* w = static_cast<WaylandWindow*>(d);
                                w->m_pointerX = wl_fixed_to_int(sx);
                                w->m_pointerY = wl_fixed_to_int(sy);
                            },
                            .button = [](void* d, wl_pointer*, uint32_t /*serial*/,
                                         uint32_t /*time*/, uint32_t btn, uint32_t state) {
                                auto* w = static_cast<WaylandWindow*>(d);
                                // Map Linux button codes to X11-style 1/2/3
                                unsigned int mapped = btn == BTN_LEFT   ? 1u
                                                    : btn == BTN_MIDDLE ? 2u
                                                    : btn == BTN_RIGHT  ? 3u
                                                    : btn;
                                if (state == WL_POINTER_BUTTON_STATE_PRESSED && w->m_onMouseDown)
                                    w->m_onMouseDown(w->m_pointerX, w->m_pointerY, mapped);
                                else if (state == WL_POINTER_BUTTON_STATE_RELEASED && w->m_onMouseUp)
                                    w->m_onMouseUp(w->m_pointerX, w->m_pointerY, mapped);
                            },
                            .axis = [](void*, wl_pointer*, uint32_t, uint32_t,
                                       wl_fixed_t) {},
                        };
                        wl_pointer_add_listener(win->m_pointer, &ptrListener, data);
                    }
                },
                .name = [](void*, wl_seat*, const char*) {}
            };
            wl_seat_add_listener(m_seat, &seatListener, this);
        }

        m_surface    = wl_compositor_create_surface(m_compositor);
        m_xdgSurface = xdg_wm_base_get_xdg_surface(m_wmBase, m_surface);

        static const xdg_surface_listener surfaceListener = {
            .configure = [](void*, xdg_surface* surf, uint32_t serial) {
                xdg_surface_ack_configure(surf, serial);
            }
        };
        xdg_surface_add_listener(m_xdgSurface, &surfaceListener, this);

        m_toplevel = xdg_surface_get_toplevel(m_xdgSurface);
        xdg_toplevel_set_title(m_toplevel, title.c_str());
        xdg_toplevel_set_app_id(m_toplevel, "singularity");

        static const xdg_toplevel_listener toplevelListener = {
            .configure = [](void*, xdg_toplevel*, int32_t, int32_t, wl_array*) {},
            .close     = [](void* data, xdg_toplevel*) {
                static_cast<WaylandWindow*>(data)->m_running = false;
            }
        };
        xdg_toplevel_add_listener(m_toplevel, &toplevelListener, this);

        wl_surface_commit(m_surface);
        wl_display_roundtrip(m_display); // process initial configure
    }

    ~WaylandWindow() override {
        if (m_pointer)    wl_pointer_destroy(m_pointer);
        if (m_seat)       wl_seat_destroy(m_seat);
        if (m_toplevel)   xdg_toplevel_destroy(m_toplevel);
        if (m_xdgSurface) xdg_surface_destroy(m_xdgSurface);
        if (m_surface)    wl_surface_destroy(m_surface);
        if (m_wmBase)     xdg_wm_base_destroy(m_wmBase);
        if (m_compositor) wl_compositor_destroy(m_compositor);
        if (m_registry)   wl_registry_destroy(m_registry);
        if (m_display)    wl_display_disconnect(m_display);
    }

    WaylandWindow(const WaylandWindow&) = delete;
    WaylandWindow& operator=(const WaylandWindow&) = delete;

    void run() override {
        m_running = true;
        while (m_running && wl_display_dispatch(m_display) != -1) {}
    }

    void close()  override { m_running = false; }
    int  width()  const override { return m_width; }
    int  height() const override { return m_height; }

    void setOnMouseDown(std::function<void(int x, int y, unsigned int button)> cb) override { m_onMouseDown = std::move(cb); }
    void setOnMouseUp(std::function<void(int x, int y, unsigned int button)> cb)   override { m_onMouseUp   = std::move(cb); }

    wl_display*  display() const { return m_display; }
    wl_surface*  surface() const { return m_surface; }

private:
    wl_display*    m_display    = nullptr;
    wl_registry*   m_registry   = nullptr;
    wl_compositor* m_compositor = nullptr;
    xdg_wm_base*   m_wmBase     = nullptr;
    wl_seat*       m_seat       = nullptr;
    wl_pointer*    m_pointer    = nullptr;
    wl_surface*    m_surface    = nullptr;
    xdg_surface*   m_xdgSurface = nullptr;
    xdg_toplevel*  m_toplevel   = nullptr;

    int  m_width    = 0;
    int  m_height   = 0;
    int  m_pointerX = 0;
    int  m_pointerY = 0;
    bool m_running  = false;

    std::function<void(int, int, unsigned int)> m_onMouseDown;
    std::function<void(int, int, unsigned int)> m_onMouseUp;
};
