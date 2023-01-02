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
enum class pd_supply_type : uint8_t {
    /// Fixed supply (Vmin = Vmax)
    fixed = 0,
    /// Battery
    battery = 1,
    /// Variable supply (non-battery)
    variable = 2,
    /// Programmable power supply
    pps = 3
};

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
    /// Minimum voltage for variable supplies (in mV)
    uint16_t min_voltage;
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

/**
 * USB PD power sink.
 * 
 * Implements the USB-PD protocol and the negotiation with a power source.
 */
struct pd_sink {
    typedef void (*event_callback)(callback_event event);

    /**
     * Initialize sink and start listening for USB-PD messages.
     */
    void init();

    /**
     * Sets the handler to be notified about USB-PD events.
     * 
     * Set the handler before calling `init()`.
     * 
     * @param cb 
     */
    void set_event_callback(event_callback cb);

    /**
     * Polls the power sink for events.
     * 
     * This function needs to be called regularly from the main loop.
     */
    void poll();

    /**
     * Requests the specified voltage from the source.
     *
     * The source will respond with `accepted` and `ps_ready` (if successful)
     * or `rejected` if unsucessful. Separate events will be triggered for these
     * messages.
     *
     * If the source hasn't advertised a matching voltage, no message is sent and
     * -1 is returned.
     * 
     * If a programmable power supply (PPS) capability is selected, the sink starts
     * to send a `request` message every 8 seconds as required by the standard.
     * Otherwise, the source will revert to 5V after 10 seconds.
     *
     * If the sink draws more power the specified maximum current, a PPS capability will
     * reduce the voltage until the current is no longer exceeded. A fixed supply uses
     * the specified current to distribute the current between multiple outputs. If
     * exceed, it might revert to 5V or stop supplying power altogether.
     * 
     * @param voltage the desired voltage (in mV)
     * @param max_current the highest current (in mA) the sink will draw,
     *   or 0 for the maximum current the source can provide for the selected voltage
     * @return index of selected source capability, or -1 if no matching voltage was found.
     */
    int request_power(int voltage, int max_current = 0);

    /**
     * Requests the specified voltage from the specified source capability.
     * 
     * The source will respond with `accepted` and `ps_ready` (if successful)
     * or `rejected` if unsucessful. Separate events will be triggered for these
     * messages.
     * 
     * If the specified voltage or current is out of the range for the specified
     * source capability or if the index is invalid, no request is sent and -1 is returned.
     * 
     * If the sink draws more power the specified maximum current, a PPS capability will
     * reduce the voltage until the current is no longer exceeded. A fixed supply uses
     * the specified current to distribute the current between multiple outputs. If
     * exceed, it might revert to 5V or stop supplying power altogether.
     * 
     * @param index index of the source capability
     * @param voltage the desired voltage (in mV)
     * @param max_current the highest current (in mA) the sink will draw (at least 25mA)
     * @return specified, or -1 if request could not be fulfilled
     */
    int request_power_from_capability(int index, int voltage, int max_current);

    /// Active power delivery protocol
    pd_protocol protocol() { return protocol_; }

    /// Number of valid elements in `source_caps` array
    uint8_t num_source_caps = 0;

    /// Array of supply capabilities
    source_capability source_caps[10];

    /// Indicates if the source can deliver unconstrained power (e.g. a wall wart)
    bool is_unconstrained = false;

    /// Requested voltage (in mV)
    uint16_t requested_voltage = 0;

    /// Requested maximum current (in mA)
    uint16_t requested_max_current = 0;

    /// Active voltage (in mV)
    uint16_t active_voltage = 5000;

    /// Active maximum current (in mA)
    uint16_t active_max_current = 900;

private:
    void handle_msg(uint16_t header, const uint8_t* payload);
    void handle_src_cap_msg(uint16_t header, const uint8_t* payload);
    bool update_protocol();
    void notify(callback_event event);
    void set_request_payload_fixed(uint8_t* payload, int obj_pos, int voltage, int current);
    void set_request_payload_pps(uint8_t* payload, int obj_pos, int voltage, int current);

    fusb302 pd_controller;
    event_callback event_callback_ = nullptr;
    pd_protocol protocol_ = pd_protocol::usb_20;
    bool supports_ext_message = false;

    int selected_pps_index = -1;
    uint32_t next_pps_request;
};

} // namespace usb_pd

#endif
