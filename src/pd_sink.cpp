//
// USB Power Delivery Sink Using FUSB302B
// Copyright (c) 2020 Manuel Bleichenbacher
//
// Licensed under MIT License
// https://opensource.org/licenses/MIT
//
// USB PD sink handling PD messages and state changes
//

#include "pd_sink.h"

#include "pd_debug.h"

#include <string.h>

namespace usb_pd {

static char version_id[24];

void pd_sink::init()
{
    pd_controller.init();

    pd_controller.get_device_id(version_id);
    DEBUG_LOG(version_id, 0);
    DEBUG_LOG("\r\n", 0);

    pd_controller.start_sink();
    update_protocol();
}

void pd_sink::stop() { pd_controller.stop(); }

void pd_sink::set_event_callback(event_callback cb) { event_callback_ = cb; }

void pd_sink::poll()
{
    while (true) {
        pd_controller.poll();

        if (!pd_controller.has_event())
            return;

        event evt = pd_controller.pop_event();

        switch (evt.kind) {
        case event_kind::state_changed:
            if (update_protocol())
                notify(callback_event::protocol_changed);
            break;
        case event_kind::message_received:
            handle_msg(evt.msg_header, evt.msg_payload);
            break;
        default:
            break;
        }
    }
}

void pd_sink::handle_msg(uint16_t header, const uint8_t* payload)
{
    pd_msg_type type = pd_header::message_type(header);
    switch (type) {
    case pd_msg_type::data_source_capabilities:
        handle_src_cap_msg(header, payload);
        break;
    case pd_msg_type::ctrl_accept:
        notify(callback_event::power_accepted);
        break;
    case pd_msg_type::ctrl_reject:
        requested_voltage = 0;
        requested_max_current = 0;
        notify(callback_event::power_rejected);
        break;
    case pd_msg_type::ctrl_ps_ready:
        active_voltage = requested_voltage;
        active_max_current = requested_max_current;
        requested_voltage = 0;
        requested_max_current = 0;
        notify(callback_event::power_ready);
        break;
    default:
        break;
    }
}

void pd_sink::handle_src_cap_msg(uint16_t header, const uint8_t* payload)
{
    int n = pd_header::num_data_objs(header);

    num_source_caps = 0;
    is_unconstrained = false;
    supports_ext_message = false;

    for (int obj_pos = 0; obj_pos < n; obj_pos++, payload += 4) {
        if (num_source_caps >= sizeof(source_caps) / sizeof(source_caps[0]))
            break;

        uint32_t capability;
        memcpy(&capability, payload, 4);

        pd_supply_type type = static_cast<pd_supply_type>(capability >> 30);
        uint16_t max_current = (capability & 0x3ff) * 10;
        uint16_t min_voltage = ((capability >> 10) & 0x03ff) * 50;
        uint16_t voltage = ((capability >> 20) & 0x03ff) * 50;

        if (type == pd_supply_type::fixed) {
            voltage = min_voltage;

            // Fixed 5V capability contains additional information
            if (voltage == 5000) {
                is_unconstrained = (capability & (1 << 27)) != 0;
                supports_ext_message = (capability & (1 << 24)) != 0;
            }

        } else if (type == pd_supply_type::pps) {
            if ((capability & (3 << 28)) != 0)
                continue;

            max_current = (capability & 0x007f) * 50;
            min_voltage = ((capability >> 8) & 0x00ff) * 100;
            voltage = ((capability >> 17) & 0x00ff) * 100;
        }

        source_caps[num_source_caps] = {
            .supply_type = type,
            .obj_pos = static_cast<uint8_t>(obj_pos + 1),
            .max_current = max_current,
            .voltage = voltage,
            .min_voltage = min_voltage,
        };
        num_source_caps++;
    }

    notify(callback_event::source_caps_changed);
}

bool pd_sink::update_protocol()
{
    auto old_protocol = protocol_;

    if (pd_controller.state() == fusb302_state::usb_pd) {
        protocol_ = pd_protocol::usb_pd;
    } else {
        protocol_ = pd_protocol::usb_20;
        active_voltage = 5000;
        active_voltage = 900;
        num_source_caps = 0;
    }

    return protocol_ != old_protocol;
}

void pd_sink::request_power(int voltage, int max_current)
{
    // Lookup object position by voltage
    int obj_pos = -1;
    for (int i = 0; i < num_source_caps; i++) {
        auto cap = source_caps + i;
        if (cap->supply_type != pd_supply_type::pps && voltage <= cap->voltage && voltage >= cap->min_voltage ) {
            obj_pos = cap->obj_pos;
            if (max_current == 0)
                max_current = source_caps[i].max_current;
        }
    }

    if (obj_pos == -1) {
        DEBUG_LOG("Unsupported voltage requested", 0);
        return; // no match
    }

    // Create 'request' message
    const uint8_t no_usb_suspend = 1;
    const uint8_t usb_comm_capable = 2;
    uint8_t payload[4];

    uint16_t curr = (max_current + 5) / 10;
    if (curr > 0x3ff)
        curr = 0x3ff;
    payload[0] = curr & 0xff;
    payload[1] = ((curr >> 8) & 0x03) | ((curr << 2) & 0xfc);
    payload[2] = (curr >> 6) & 0x0f;
    payload[3] = (obj_pos & 0x07) << 4 | no_usb_suspend | usb_comm_capable;
    uint16_t header = pd_header::create_data(pd_msg_type::data_request, 1);

    // Send message
    pd_controller.send_message(header, payload);

    requested_voltage = voltage;
    requested_max_current = max_current;
}

void pd_sink::notify(callback_event event)
{
    if (event_callback_ == nullptr)
        return;
    event_callback_(event);
}

} // namespace usb_pd
