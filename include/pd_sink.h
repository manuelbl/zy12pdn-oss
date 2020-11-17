//
// USB Power Delivery Sink Using FUSB302B
// Copyright (c) 2020 Manuel Bleichenbacher
//
// Licensed under MIT License
// https://opensource.org/licenses/MIT
//
// USB PD sink handling PD messages and state changes
//

#ifndef _pd_sink_h_
#define _pd_sink_h_

#include "fusb302.h"

namespace usb_pd {

/// Power supply type
enum class pd_supply_type : uint8_t { fixed = 0, battery = 1, variable = 2, augmented = 3 };

enum class pd_attach_state { usb_20, usb_pd };

/// Power source capability
struct source_capability {
    /// Supply type (fixed, batttery, variable etc.)
    pd_supply_type supply_type;
    /// Position within message (don't touch)
    uint8_t obj_pos;
    /// Maximum current (in mA)
    uint16_t max_current;
    /// Voltage (in mV)
    uint16_t voltage;
};

struct pd_sink {
    typedef void (*attach_state_changed_callback)();
    typedef void (*source_caps_changed_callback)();

    void init();
    void stop();

    void set_attach_state_changed_callback(attach_state_changed_callback cb);
    void set_source_caps_changed_callback(source_caps_changed_callback cb);

    void poll();

    pd_attach_state attach_state() { return attach_state_; }

    void request_power(int voltage, int max_current);

    /// Number of valid source capabilities in `source_caps` array
    uint8_t num_source_caps = 0;

    /// Array of supply capabilities
    source_capability source_caps[10];

private:
    void handle_msg(uint16_t header, const uint8_t* payload);
    void handle_src_cap_msg(uint16_t header, const uint8_t* payload);
    bool update_attach_state();

    fusb302 pd_controller;
    attach_state_changed_callback attach_state_changed_cb = nullptr;
    source_caps_changed_callback source_caps_changed_cb = nullptr;
    pd_attach_state attach_state_ = pd_attach_state::usb_20;
};

} // namespace usb_pd

#endif
