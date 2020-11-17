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

/// Power deliver protocol
enum class pd_protocol {
    /// No USB PD communication (5V only)
    usb_20,
    /// USB PD communication
    usb_pd
};

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

/// Callback event types
enum class callback_event {
    /// Power delivery protocol has changed
    protocol_changed,
    /// Source capabilities have changed (immediately request power)
    source_caps_changed,
    /// Requested power has been accepted (but not ready yet)
    power_accepted,
    /// Requested power has been rejected
    power_rejected,
    /// Requested power is now ready
    power_ready
};

struct pd_sink {
    typedef void (*event_callback)(callback_event event);

    void init();
    void stop();

    void set_event_callback(event_callback cb);

    void poll();

    void request_power(int voltage, int max_current);

    /// Active power delivery protocol
    pd_protocol protocol() { return protocol_; }

    /// Number of valid elements in `source_caps` array
    uint8_t num_source_caps = 0;

    /// Array of supply capabilities
    source_capability source_caps[10];

    /// Requested voltage (in mV)
    uint16_t requested_voltage = 0;

    /// Requested maximum current (in mA)
    uint16_t requested_max_current = 0;

    /// Active voltage (in mV)
    uint16_t active_voltage = 5000;

    /// Active maximum current (in mA)
    uint16_t active_max_current = 500;

private:
    void handle_msg(uint16_t header, const uint8_t* payload);
    void handle_src_cap_msg(uint16_t header, const uint8_t* payload);
    bool update_protocol();
    void notify(callback_event event);

    fusb302 pd_controller;
    event_callback event_callback_ = nullptr;
    pd_protocol protocol_ = pd_protocol::usb_20;
};

} // namespace usb_pd

#endif
