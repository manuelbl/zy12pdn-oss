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

#include "fusb302.h"

#include <string.h>

#include "hal.h"
#include "pd_debug.h"

namespace usb_pd {

static const char* const PRODUCT_IDS[] = { "FUSB302B__X", "FUSB302B01MPX", "FUSB302B10MPX", "FUSB302B11MPX" };

static const char* VERSIONS = "????????ABCDEFGH";

void fusb302::get_device_id(char* device_id_buf)
{
    uint8_t device_id = read_register(reg::device_id);
    uint8_t version_id = device_id >> 4;
    uint8_t product_id = (device_id >> 2) & 0x03;
    uint8_t revision_id = device_id & 0x03;

    strcpy(device_id_buf, PRODUCT_IDS[product_id]);
    char piece[8] = " ._rev.";
    piece[1] = VERSIONS[version_id];
    piece[6] = 'A' + revision_id;
    strcat(device_id_buf, piece);
}

void fusb302::init()
{
    // full reset
    write_register(reg::reset, *(reg_reset::sw_res | reg_reset::pd_reset));
    hal.delay(10);

    // power up everyting except oscillator
    write_register(reg::power, *(reg_power::pwr_all & ~reg_power::pwr_int_osc));
    // Disable all CC monitoring
    write_register(reg::switches0, *reg_switches0::none);
    // Mask all interrupts
    write_register(reg::mask, *reg_mask::m_all);
    // Mask all interrupts
    write_register(reg::maska, *reg_maska::m_all);
    // Mask all interrupts (incl. good CRC sent)
    write_register(reg::maskb, *reg_maskb::m_all);

    next_message_id = 0;
    is_timeout_active = false;
    state_ = fusb302_state::usb_20;
    events.clear();
}

void fusb302::start_sink()
{
    // As the interrupt line is also used as SWDIO, the FUSB302B interrupt is
    // not activated until activity on CC1 or CC2 has been detected.
    // Thus, CC1 and CC2 have to be polled manually even though the FUSB302B
    // could do it automatically.

    // BMC threshold: 1.35V with a threshold of 85mV
    write_register(reg::slice, *reg_slice::sdac_hys_085mv | 0x20);

    start_measurement(1);
}

void fusb302::poll()
{
    if (hal.is_interrupt_asserted()) {
        check_for_interrupts();

    } else if (has_timeout_expired()) {
        if (state_ == fusb302_state::usb_pd_wait) {
            DEBUG_LOG("%lu: No CC activity\r\n", hal.millis());
            establish_retry_wait();
        } else if (state_ == fusb302_state::usb_20) {
            check_measurement();
        } else if (state_ == fusb302_state::usb_retry_wait) {
            establish_usb_20();
        }
    }
}

void fusb302::start_measurement(int cc)
{
    reg_switches0 sw0 = cc == 1 ? reg_switches0::meas_cc1 : reg_switches0::meas_cc2;
    sw0 = sw0 | reg_switches0::pdwn1 | reg_switches0::pdwn2;

    // test CC
    write_register(reg::switches0, *sw0);
    start_timeout(10);
    measuring_cc = cc;
}

void fusb302::check_measurement()
{
    read_register(reg::status0);
    reg_status0 status0 = static_cast<reg_status0>(read_register(reg::status0));
    if (*(status0 & reg_status0::bc_lvl_mask) == 0) {
        // No CC activity
        start_measurement(measuring_cc == 1 ? 2 : 1);
        return;
    }

    establish_usb_pd_wait(measuring_cc);
    measuring_cc = 0;
}

void fusb302::check_for_interrupts()
{
    bool may_have_message = false;

    reg_interrupt interrupt = static_cast<reg_interrupt>(read_register(reg::interrupt));
    reg_interrupta interrupta = static_cast<reg_interrupta>(read_register(reg::interrupta));
    reg_interruptb interruptb = static_cast<reg_interruptb>(read_register(reg::interruptb));

    if (*(interrupta & reg_interrupta::i_hardrst) != 0) {
        DEBUG_LOG("%lu: Hard reset\r\n", hal.millis());
        establish_retry_wait();
        return;
    }
    if (*(interrupta & reg_interrupta::i_retryfail) != 0) {
        DEBUG_LOG("Retry failed\r\n", 0);
    }
    if (*(interrupta & reg_interrupta::i_txsent) != 0) {
        DEBUG_LOG("TX ack\r\n", 0);
        // turn off internal oscillator if TX FIFO is empty
        reg_status1 status1 = static_cast<reg_status1>(read_register(reg::status1));
        if (*(status1 & reg_status1::tx_empty) != 0)
            write_register(reg::power, *(reg_power::pwr_all & ~reg_power::pwr_int_osc));
    }
    if (*(interrupt & reg_interrupt::i_activity) != 0) {
        may_have_message = true;
    }
    if (*(interrupt & reg_interrupt::i_crc_chk) != 0) {
        // DEBUG_LOG("%lu: CRC ok\r\n", hal.millis());
        may_have_message = true;
    }
    if (*(interruptb & reg_interruptb::i_gcrcsent) != 0) {
        // DEBUG_LOG("Good CRC sent\r\n", 0);
        may_have_message = true;
    }
    if (may_have_message)
        check_for_msg();
}

void fusb302::check_for_msg()
{
    while (true) {
        reg_status1 status1 = static_cast<reg_status1>(read_register(reg::status1));
        if ((status1 & reg_status1::rx_empty) == reg_status1::rx_empty)
            break;

        uint16_t header;
        uint8_t* payload = rx_message_buf[rx_message_index];
        read_message(header, payload);

        reg_status0 status0 = static_cast<reg_status0>(read_register(reg::status0));
        if (*(status0 & reg_status0::crc_chk) == 0) {
            DEBUG_LOG("Invalid CRC\r\n", 9);
        } else if (pd_header::message_type(header) == pd_msg_type::ctrl_good_crc) {
            DEBUG_LOG("Good CRC packet\r\n", 9);
        } else {
            if (state_ != fusb302_state::usb_pd)
                establish_usb_pd();
            events.add_item(event(header, payload));
            rx_message_index++;
            if (rx_message_index >= num_message_buf)
                rx_message_index = 0;
        }
    }
}

void fusb302::establish_retry_wait()
{
    DEBUG_LOG("Reset\r\n", 0);

    // Reset FUSB302
    init();
    state_ = fusb302_state::usb_retry_wait;
    start_timeout(500);
    events.add_item(event(event_kind::state_changed));
}

void fusb302::establish_usb_20() { start_sink(); }

void fusb302::establish_usb_pd_wait(int cc)
{
    // Configure INT_N pin
    hal.init_int_n();

    // Enable automatic retries
    write_register(reg::control3, *(reg_control3::auto_retry | reg_control3::_3_retries));
    // Enable interrupts for CC activity and CRC_CHK
    write_register(reg::mask, *(reg_mask::m_all & ~(reg_mask::m_activity | reg_mask::m_crc_chk)));
    // Unmask all interrupts (toggle done, hard reset, tx sent etc.)
    write_register(reg::maska, *reg_maska::m_none);
    // Enable good CRC sent interrupt
    write_register(reg::maskb, *reg_maskb::m_none);
    // Enable pull down and CC monitoring
    write_register(reg::switches0,
        *(reg_switches0::pdwn1 | reg_switches0::pdwn2 | (cc == 1 ? reg_switches0::meas_cc1 : reg_switches0::meas_cc2)));
    // Configure: auto CRC and BMC transmit on CC pin
    write_register(reg::switches1,
        *(reg_switches1::specrev_rev_2_0 | reg_switches1::auto_crc
            | (cc == 1 ? reg_switches1::txcc1 : reg_switches1::txcc2)));
    // Enable interrupt
    write_register(reg::control0, *reg_control0::none);

    state_ = fusb302_state::usb_pd_wait;
    start_timeout(300);
}

void fusb302::establish_usb_pd()
{
    state_ = fusb302_state::usb_pd;
    cancel_timeout();
    DEBUG_LOG("USB PD comm\r\n", 0);
    events.add_item(event(event_kind::state_changed));
}

void fusb302::start_timeout(uint32_t ms)
{
    is_timeout_active = true;
    timeout_expiration = hal.millis() + ms;
}

bool fusb302::has_timeout_expired()
{
    if (!is_timeout_active)
        return false;

    uint32_t delta = timeout_expiration - hal.millis();
    if (delta <= 0x8000000)
        return false;

    is_timeout_active = false;
    return true;
}

void fusb302::cancel_timeout() { is_timeout_active = false; }

bool fusb302::has_event() { return events.num_items() != 0; }

event fusb302::pop_event() { return events.pop_item(); }

uint8_t fusb302::read_message(uint16_t& header, uint8_t* payload)
{
    // Read token and header
    uint8_t buf[3];
    hal.pd_ctrl_read(static_cast<uint8_t>(reg::fifos), 3, buf);

    // Check for SOP token
    if ((buf[0] & 0xe0) != 0xe0) {
        // Flush RX FIFO
        write_register(reg::control1, *reg_control1::rx_flush);
        return 0;
    }

    uint8_t* header_buf = reinterpret_cast<uint8_t*>(&header);
    header_buf[0] = buf[1];
    header_buf[1] = buf[2];

    // Get payload and CRC length
    uint8_t len = pd_header::num_data_objs(header) * 4;
    hal.pd_ctrl_read(static_cast<uint8_t>(reg::fifos), len + 4, payload);

    return len;
}

void fusb302::send_header_message(pd_msg_type msg_type)
{
    uint16_t header = pd_header::create_ctrl(msg_type);
    send_message(header, nullptr);
}

void fusb302::send_message(uint16_t header, const uint8_t* payload)
{
    // Enable internal oscillator
    write_register(reg::power, *reg_power::pwr_all);

    int payload_len = pd_header::num_data_objs(header) * 4;
    header |= (next_message_id << 9);

    uint8_t buf[40];

    // Create token stream
    buf[0] = *token::sop1;
    buf[1] = *token::sop1;
    buf[2] = *token::sop1;
    buf[3] = *token::sop2;
    buf[4] = *token::packsym | static_cast<uint8_t>(payload_len + 2);
    buf[5] = header & 0xff;
    buf[6] = header >> 8;
    if (payload_len > 0)
        memcpy(buf + 7, payload, payload_len);
    int n = 7 + payload_len;
    buf[n++] = *token::jam_crc;
    buf[n++] = *token::eop;
    buf[n++] = *token::txoff;
    buf[n++] = *token::txon;

    hal.pd_ctrl_write(static_cast<uint8_t>(reg::fifos), n, buf);

    next_message_id++;
    if (next_message_id == 8)
        next_message_id = 0;
}

uint8_t fusb302::read_register(reg reg_addr)
{
    uint8_t val;
    hal.pd_ctrl_read(static_cast<uint8_t>(reg_addr), 1, &val);
    return val;
}

void fusb302::read_registers(reg start_addr, int n, uint8_t* target)
{
    hal.pd_ctrl_read(static_cast<uint8_t>(start_addr), n, target);
}

void fusb302::write_register(reg reg_addr, uint8_t value)
{
    hal.pd_ctrl_write(static_cast<uint8_t>(reg_addr), 1, &value);
}

} // namespace usb_pd
