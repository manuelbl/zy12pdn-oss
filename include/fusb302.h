//
// USB Power Delivery Sink Using FUSB302B
// Copyright (c) 2020 Manuel Bleichenbacher
//
// Licensed under MIT License
// https://opensource.org/licenses/MIT
//
// FUSB302 communication incl. attach state management
// and decoding/encoding of messages.
//

#pragma once

#include "fusb302_regs.h"
#include "hal.h"
#include "queue.h"
#include "usb_pd.h"

namespace usb_pd {

/// FUSB302 state
enum class fusb302_state {
    /// VBUS is present, monitoring for activity on CC1/CC2
    usb_20,
    /// Activity on CC1/CC2 has been detected, waiting for first USB PD message
    usb_pd_wait,
    /// Successful USB PD communication established
    usb_pd,
    /// Wait period after a failure
    usb_retry_wait
};

/// Event kind
enum class event_kind { none, state_changed, message_received };

/// Event queue by FUSB302 instance for clients (such as `pd_sink`)
struct event {
    /// Event kind
    event_kind kind;

    /// Message header (valid if event_kind = `message_received`)
    uint16_t msg_header;

    /// Message payload (valid if event_kind = `message_received`, possibly `null`)
    const uint8_t* msg_payload;

    event() : kind(event_kind::none) { }

    event(event_kind evt_kind) : kind(evt_kind) { }

    event(uint16_t header, const uint8_t* payload = nullptr)
        : kind(event_kind::message_received)
        , msg_header(header)
        , msg_payload(payload)
    { }
};

/**
 * FUSB302 instance.
 *
 * Implements communication with FUSB302, manages attachment state (suitable
 * for a sink), decodes and encode messages.
 *
 * `poll()` must be called at least every 1ms. After the call, the instance
 * can be checked for events.
 *
 * Internally, a short queue is used to store the events until they have been
 * consumed.
 */
struct fusb302 {
    /**
     * Gets the device ID.
     * @param device_id_buf buffer receiving device ID string (at least 24 bytes long)
     */
    void get_device_id(char* device_id_buf);

    /// Initializes the FUSB302 and the communication (without starting it)
    void init();

    /**
     * Starts the FUSB302 in the role of a sink.
     *
     * It will enable the pull-down resistors and monitor CC1 and CC2 for USB-PD
     * communication. Once a source has connected, the appropriate CC line will
     * be configured for communication.
     */
    void start_sink();

    /**
     * Polls FUSB302 for interrupts and messages.
     *
     * After a call to this functions, new events may be available.
     */
    void poll();

    /// Gets the current protocol state.
    fusb302_state state() { return state_; }

    /**
     * Sends a message with the given header and payload.
     * The message ID is automatically inserted into the header.
     */
    void send_message(uint16_t header, const uint8_t* payload);

    /**
     * Sends a message of type 'msg_type'.
     * Only suitable for messages without payload.
     */
    void send_header_message(pd_msg_type msg_type);

    /// Indicates if an event is available.
    bool has_event();

    /// Retrieves the oldest event and removes it from the queue
    event pop_event();

private:
    void check_for_interrupts();
    void check_for_msg();
    void start_measurement(int cc);
    void check_measurement();
    void establish_usb_20();
    void establish_usb_pd_wait(int cc);
    void establish_usb_pd();
    void establish_retry_wait();

    /// Checks if the timeout has expired
    bool has_timeout_expired();
    /// Starts a new timeout (and cancels the pending one)
    void start_timeout(uint32_t ms);
    /// Cancels the pending timeout (if any)
    void cancel_timeout();

    /// Retrieves the received message from the FIFO into the specified variables.
    uint8_t read_message(uint16_t& header, uint8_t* payload);

    /// Reads the value of the specified register.
    uint8_t read_register(reg r);

    /// Reads the values of several consecutive registers.
    void read_registers(reg start_addr, int n, uint8_t* target);

    /// Write a value to the specified register.
    void write_register(reg r, uint8_t value);

    /// cc line being measured
    int measuring_cc = 0;

    /// Indicates if the timeout timer is running
    bool is_timeout_active = false;

    /// Time when the current timer expires
    uint32_t timeout_expiration;

    constexpr static int num_message_buf = 4;

    /// RX message buffers
    uint8_t rx_message_buf[64][num_message_buf];

    /// Next RX message index
    int rx_message_index = 0;

    /// Queue of event that have occurred
    queue<event, 6> events;

    /// Current attachment state
    fusb302_state state_ = fusb302_state::usb_20;

    /// ID for next USB PD message
    uint16_t next_message_id = 0;
};

} // namespace usb_pd
