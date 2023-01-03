//
// USB Power Delivery Sink Using FUSB302B
// Copyright (c) 2020 Manuel Bleichenbacher
//
// Licensed under MIT License
// https://opensource.org/licenses/MIT
//

#pragma once

#include <stdint.h>

namespace usb_pd {

/// USB PD message type
enum pd_msg_type : uint8_t {
    pd_msg_type_ctrl_good_crc = 0x01,
    pd_msg_type_ctrl_goto_min = 0x02,
    pd_msg_type_ctrl_accept = 0x03,
    pd_msg_type_ctrl_reject = 0x04,
    pd_msg_type_ctrl_ping = 0x05,
    pd_msg_type_ctrl_ps_ready = 0x06,
    pd_msg_type_ctrl_get_source_cap = 0x07,
    pd_msg_type_ctrl_get_sink_cap = 0x08,
    pd_msg_type_ctrl_dr_swap = 0x09,
    pd_msg_type_ctrl_pr_swap = 0x0a,
    pd_msg_type_ctrl_vconn_swap = 0x0b,
    pd_msg_type_ctrl_wait = 0x0c,
    pd_msg_type_ctrl_soft_reset = 0x0d,
    pd_msg_type_ctrl_data_reset = 0x0e,
    pd_msg_type_ctrl_data_reset_complete = 0x0f,
    pd_msg_type_ctrl_not_supported = 0x10,
    pd_msg_type_ctrl_get_source_cap_extended = 0x11,
    pd_msg_type_ctrl_get_status = 0x12,
    pd_msg_type_ctrl_fr_swap = 0x13,
    pd_msg_type_ctrl_get_pps_status = 0x14,
    pd_msg_type_ctrl_get_country_codes = 0x15,
    pd_msg_type_ctrl_get_sink_cap_extended = 0x16,
    pd_msg_type_data_source_capabilities = 0x81,
    pd_msg_type_data_request = 0x82,
    pd_msg_type_data_bist = 0x83,
    pd_msg_type_data_sink_capabilities = 0x84,
    pd_msg_type_data_battery_status = 0x85,
    pd_msg_type_data_alert = 0x86,
    pd_msg_type_data_get_country_info = 0x87,
    pd_msg_type_data_enter_usb = 0x88,
    pd_msg_type_data_vendor_defined = 0x8f
};

/// Helper class to constrcut and decode USB PD message headers
struct pd_header {
    static bool has_extended(uint16_t header) { return (header & 0x8000) != 0; }
    static int num_data_objs(uint16_t header) { return (header >> 12) & 0x07; }
    static uint8_t message_id(uint16_t header) { return (header >> 9) & 0x07; }

    static pd_msg_type message_type(uint16_t header) {
        return static_cast<pd_msg_type>(((num_data_objs(header) != 0) << 7) | (header & 0x1f));
    }

    static int spec_rev(uint16_t header) { return ((header >> 6) & 0x03) + 1; }

    static uint16_t create_ctrl(pd_msg_type msg_type, int rev = 1) {
        return (msg_type & 0x1f) | 0x40 | ((rev - 1) << 6);
    }

    static uint16_t create_data(pd_msg_type msg_type, int num_data_objs, int rev = 1) {
        return ((num_data_objs & 0x07) << 12) | (msg_type & 0x1f) | 0x40 | ((rev - 1) << 6);
    }
};

} // namespace usb_pd
