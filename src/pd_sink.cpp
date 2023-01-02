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

#include "hal.h"
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

void pd_sink::set_event_callback(event_callback cb) { event_callback_ = cb; }

void pd_sink::poll()
{
    // process events from PD controller
    while (true) {
        pd_controller.poll();

        if (!pd_controller.has_event())
            break;

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

    // check if it is time to re-request PPS voltage
    if (selected_pps_index != -1 && requested_voltage == 0 && hal.has_expired(next_pps_request))
        request_power_from_capability(selected_pps_index, active_voltage, active_max_current);
}

void pd_sink::handle_msg(uint16_t header, const uint8_t* payload)
{
    spec_rev = pd_header::spec_rev(header);

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
        selected_pps_index = -1;
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
        active_max_current = 900;
        num_source_caps = 0;
    }

    return protocol_ != old_protocol;
}

int pd_sink::request_power(int voltage, int max_current)
{
    // Lookup fixed voltage capabilities first
    int index = -1;
    for (int i = 0; i < num_source_caps; i++) {
        auto cap = source_caps + i;
        if (cap->supply_type == pd_supply_type::fixed
                && voltage >= cap->min_voltage && voltage <= cap->voltage) {
            index = i;
            if (max_current == 0)
                max_current = cap->max_current;
            break;
        }
    }

    // Lookup PPS capabilites next
    if (index == -1) {
        for (int i = 0; i < num_source_caps; i++) {
            auto cap = source_caps + i;
            if (cap->supply_type == pd_supply_type::pps
                    && voltage >= cap->min_voltage && voltage <= cap->voltage) {
                if (max_current == 0) {
                    max_current = cap->max_current;
                    index = i;
                    break;
                } else if (max_current >= 25 && max_current <= cap->max_current) {
                    index = i;
                    break;
                }
            }
        }
    }

    if (index == -1) {
        DEBUG_LOG("Unsupported voltage %d requested", voltage);
        return -1; // no match
    }

    return request_power_from_capability(index, voltage, max_current);
}

int pd_sink::request_power_from_capability(int index, int voltage, int max_current)
{
    if (index < 0 || index >= num_source_caps)
        return -1;
    auto cap = source_caps + index;
    if (cap->supply_type != pd_supply_type::fixed && cap->supply_type != pd_supply_type::pps)
        return -1;
    if (voltage < cap->min_voltage || voltage > cap->voltage)
        return -1;
    if (max_current < 25 || max_current > cap->max_current)
        return -1;

    // Create 'request' message
    uint8_t payload[4];
    if (cap->supply_type == pd_supply_type::fixed) {
        set_request_payload_fixed(payload, cap->obj_pos, voltage, max_current);
        selected_pps_index = -1;
    } else {
        set_request_payload_pps(payload, cap->obj_pos, voltage, max_current);
        selected_pps_index = index;
        next_pps_request = hal.millis() + 8000;
    }

    uint16_t header = pd_header::create_data(pd_msg_type::data_request, 1, spec_rev);

    // Send message
    pd_controller.send_message(header, payload);

    return cap->obj_pos;
}

void pd_sink::set_request_payload_fixed(uint8_t* payload, int obj_pos, int voltage, int current)
{
    const uint8_t no_usb_suspend = 1;
    const uint8_t usb_comm_capable = 2;

    current = (current + 5) / 10;
    if (current > 0x3ff)
        current = 0x3ff;
    payload[0] = current & 0xff;
    payload[1] = ((current >> 8) & 0x03) | ((current << 2) & 0xfc);
    payload[2] = (current >> 6) & 0x0f;
    payload[3] = (obj_pos & 0x07) << 4 | no_usb_suspend | usb_comm_capable;

    requested_voltage = voltage;
    requested_max_current = current * 10;
}

void pd_sink::set_request_payload_pps(uint8_t* payload, int obj_pos, int voltage, int current)
{
    const uint8_t no_usb_suspend = 1;
    const uint8_t usb_comm_capable = 2;

    current = (current + 25) / 50;
    if (current > 0x7f)
        current = 0x7f;
    voltage = (voltage + 10) / 20;
    if (voltage > 0x7ff)
        voltage = 0x7ff;
    payload[0] = current;
    payload[1] = voltage << 1;
    payload[2] = (voltage >> 7) & 0x0f;
    payload[3] = (obj_pos & 0x07) << 4 | no_usb_suspend | usb_comm_capable;

    requested_voltage = voltage * 20;
    requested_max_current = current * 50;
}

void pd_sink::notify(callback_event event)
{
    if (event_callback_ == nullptr)
        return;
    event_callback_(event);
}

} // namespace usb_pd
