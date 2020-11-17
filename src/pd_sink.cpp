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

namespace usb_pd {

static char version_id[24];

void pd_sink::init()
{
    pd_controller.init();

    pd_controller.get_device_id(version_id);
    DEBUG_LOG(version_id, 0);
    DEBUG_LOG("\r\n", 0);

    pd_controller.start_sink();
    update_attach_state();
}

void pd_sink::stop() { pd_controller.stop(); }

void pd_sink::set_attach_state_changed_callback(attach_state_changed_callback cb) { attach_state_changed_cb = cb; }

void pd_sink::set_source_caps_changed_callback(source_caps_changed_callback cb) { source_caps_changed_cb = cb; }

void pd_sink::poll()
{
    while (true) {
        pd_controller.poll();

        if (!pd_controller.has_event())
            return;
        event evt = pd_controller.pop_event();

        switch (evt.kind) {
        case event_kind::state_changed:
            if (update_attach_state() && attach_state_changed_cb != nullptr)
                attach_state_changed_cb();
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
    default:
        break;
    }
}

void pd_sink::handle_src_cap_msg(uint16_t header, const uint8_t* payload)
{
    int n = pd_header::num_data_objs(header);

    num_source_caps = 0;

    for (int obj_pos = 0; obj_pos < n; obj_pos++) {
        if (num_source_caps >= sizeof(source_caps) / sizeof(source_caps[0]))
            break;

        pd_supply_type type = static_cast<pd_supply_type>(payload[3] >> 6);
        uint16_t max_current = 0;
        uint16_t voltage = 0;

        if (type == pd_supply_type::fixed) {
            voltage = ((payload[2] & 0x0f) * 64 + (payload[1] >> 2)) * 50;
            max_current = ((payload[1] & 0x03) * 256 + payload[0]) * 10;
        }

        source_caps[num_source_caps] = {
            .supply_type = type,
            .obj_pos = static_cast<uint8_t>(obj_pos + 1),
            .max_current = max_current,
            .voltage = voltage,
        };
        num_source_caps++;

        payload += 4;
    }

    if (source_caps_changed_cb != nullptr)
        source_caps_changed_cb();
}

bool pd_sink::update_attach_state()
{
    bool needs_notify = true;

    fusb302_state state = pd_controller.attach_state();
    switch (state) {
    case fusb302_state::usb_20:
        attach_state_ = pd_attach_state::usb_20;
        break;
    case fusb302_state::usb_pd:
        attach_state_ = pd_attach_state::usb_pd;
        break;
    default:
        needs_notify = false;
    }

    return needs_notify;
}

void pd_sink::request_power(int voltage, int max_current)
{
    // Lookup object position by voltage
    int obj_pos = -1;
    for (int i = 0; i < num_source_caps; i++)
        if (source_caps[i].voltage == voltage)
            obj_pos = source_caps[i].obj_pos;
    if (obj_pos == -1)
        return; // no match

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
}

} // namespace usb_pd
