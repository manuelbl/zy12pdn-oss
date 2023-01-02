//
// USB Power Delivery Sink Using FUSB302B
// Copyright (c) 2020 Manuel Bleichenbacher
//
// Licensed under MIT License
// https://opensource.org/licenses/MIT
//

#ifndef _usb_pd_h_
#define _usb_pd_h_

#include <stdint.h>

namespace usb_pd {

/// USB PD message type
enum class pd_msg_type : uint8_t {
    ctrl_good_crc = 0x01,
    ctrl_goto_min = 0x02,
    ctrl_accept = 0x03,
    ctrl_reject = 0x04,
    ctrl_ping = 0x05,
    ctrl_ps_ready = 0x06,
    ctrl_get_source_cap = 0x07,
    ctrl_get_sink_cap = 0x08,
    ctrl_dr_swap = 0x09,
    ctrl_pr_swap = 0x0a,
    ctrl_vconn_swap = 0x0b,
    ctrl_wait = 0x0c,
    ctrl_soft_reset = 0x0d,
    ctrl_data_reset = 0x0e,
    ctrl_data_reset_complete = 0x0f,
    ctrl_not_supported = 0x10,
    ctrl_get_source_cap_extended = 0x11,
    ctrl_get_status = 0x12,
    ctrl_fr_swap = 0x13,
    ctrl_get_pps_status = 0x14,
    ctrl_get_country_codes = 0x15,
    ctrl_get_sink_cap_extended = 0x16,
    data_source_capabilities = 0x81,
    data_request = 0x82,
    data_bist = 0x83,
    data_sink_capabilities = 0x84,
    data_battery_status = 0x85,
    data_alert = 0x86,
    data_get_country_info = 0x87,
    data_enter_usb = 0x88,
    data_vendor_defined = 0x8f
};

constexpr uint8_t operator*(pd_msg_type msg_type) { return static_cast<uint8_t>(msg_type); }

/// Helper class to constrcut and decode USB PD message headers
struct pd_header {
    static bool has_extended(uint16_t header) { return (header & 0x8000) != 0; }
    static int num_data_objs(uint16_t header) { return (header >> 12) & 0x07; }
    static uint8_t message_id(uint16_t header) { return (header >> 9) & 0x07; }
    static pd_msg_type message_type(uint16_t header)
    {
        return static_cast<pd_msg_type>((num_data_objs(header) != 0) << 7 | (header & 0x1f));
    }
    static int spec_rev(uint16_t header) { return ((header >> 6) & 0x03) + 1; }
    static uint16_t create_ctrl(pd_msg_type msg_type, int rev = 1)
    {
        return (*msg_type & 0x1f) | 0x40 | ((rev - 1) << 6);
    }
    static uint16_t create_data(pd_msg_type msg_type, int num_data_objs, int rev = 1)
    {
        return ((num_data_objs & 0x07) << 12) | (*msg_type & 0x1f) | 0x40 | ((rev - 1) << 6);
    }
};

} // namespace usb_pd

#endif
