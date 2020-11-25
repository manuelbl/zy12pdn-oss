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

#ifndef _fusb302_h_
#define _fusb302_h_

#include "fusb302_regs.h"
#include "hal.h"
#include "queue.h"
#include "usb_pd.h"

namespace usb_pd {

/// FUSB302 state
enum class fusb302_state : uint8_t {
    /// VBUS is present, no activity on CC1/CC2
    usb_20,
    /// Activity on CC1/CC2, waiting to receive USB PD messages
    usb_pd_wait,
    /// Successful USB PD communication established
    usb_pd,
    /// Hard reset has been sent
    hard_reset_sent
};

/// Event kind
enum class event_kind : uint8_t { none, state_changed, message_received };

/// Event
struct event {
    /// EVent kind
    event_kind kind;

    /// Message header (valid if event_kind = `message_received`)
    uint16_t msg_header;

    /// Message payload (valid if event_kind = `message_received`, possibly `null`)
    const uint8_t* msg_payload;

    event()
        : kind(event_kind::none)
    {
    }
    event(event_kind evt_kind)
        : kind(evt_kind)
    {
    }
    event(uint16_t header, const uint8_t* payload = nullptr)
        : kind(event_kind::message_received)
        , msg_header(header)
        , msg_payload(payload)
    {
    }
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
     * Stops the FUSB302.
     *
     * It will neither behave as a sink nor as a source and will
     * no longer receive messages or be able to send messages.
     */
    void stop();

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
    void establish_usb_20();
    void establish_usb_pd_wait();
    void establish_usb_pd();
    void send_hard_reset();

    /// Update the state based on the FUSB302 status
    void update_state(bool timeout_ended);

    /// Gets what CC line has activity (if any)
    int toggle_state();

    bool has_timeout_expired();
    void start_timeout(uint32_t ms);
    void cancel_timeout();

    /// Retrieves the received message from the FIFO into the specified variables.
    uint8_t read_message(uint16_t& header, uint8_t* payload);

    /// Reads the value of the specified register.
    uint8_t read_register(reg reg_addr);

    /// Reads the values of several consecutive registers.
    void read_registers(reg start_addr, int n, uint8_t* target);

    /// Write a value to the specified register.
    void write_register(reg reg_addr, uint8_t value);

    /// Indicates if the timeout timer is running
    bool is_timeout_active = false;

    /// Time when the current timer expires
    uint32_t timeout_expiration;

    /// Messages buffers
    uint8_t message_buf[64][4];

    /// Next message index
    int message_index = 0;

    /// Queue of event that have occurred
    queue<event, 6> events;

    /// Current attachment state
    fusb302_state state_ = fusb302_state::usb_20;

    /// ID for next USB PD message
    uint16_t next_message_id = 0;
};

} // namespace usb_pd

#endif
