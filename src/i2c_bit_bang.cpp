// USB Power Delivery Sink Using FUSB302B
// Copyright (c) 2020 Manuel Bleichenbacher
//
// Licensed under MIT License
// https://opensource.org/licenses/MIT
//
// I2C bit banging
//

#include "i2c_bit_bang.h"

#include <libopencm3/stm32/rcc.h>

namespace usb_pd {

void i2c_bit_bang::init() {
    // SCL is driven high and low as the board has no pull-up resistor.
    // Therefore, clock stretching is not supported.
    gpio_set(scl_port, scl_pin);
    gpio_mode_setup(scl_port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, scl_pin);
    gpio_set_output_options(scl_port, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, scl_pin);

    // SDA is configured as open-drain, so it's only driven on low.
    gpio_clear(sda_port, sda_pin);
    gpio_mode_setup(sda_port, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, sda_pin);
    gpio_set_output_options(sda_port, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, sda_pin);
}

bool i2c_bit_bang::write_data(uint8_t addr, uint8_t reg, int data_len, const uint8_t* data, bool end_with_stop) {
    write_start_cond();
    bool ack = write_byte(addr << 1);
    ack = ack && write_byte(reg);
    for (int i = 0; i < data_len; i++) {
        ack = ack && write_byte(data[i]);
    }
    if (end_with_stop && !ack)
        write_stop_cond();
    return ack;
}

bool i2c_bit_bang::read_data(uint8_t addr, uint8_t reg, int data_len, uint8_t* data) {
    bool ack = write_data(addr, reg, 0, nullptr, false);

    if (ack)
        write_start_cond();
    ack = ack && write_byte((addr << 1) | 1);
    for (int i = 0; i < data_len; i++) {
        data[i] = read_byte(i == data_len - 1);
    }
    write_stop_cond();
    return ack;
}

void i2c_bit_bang::write_start_cond() {
    if (is_started) {
        set_sda();
        delay();
        set_scl();
        delay();
    }

    clear_sda();
    delay();
    clear_scl();
    is_started = true;
}

void i2c_bit_bang::write_stop_cond() {
    clear_sda();
    delay();
    set_scl();
    delay();
    set_sda();
    delay();
    is_started = false;
}

bool i2c_bit_bang::write_byte(uint8_t value) {
    for (int i = 0; i < 8; i++) {
        write_bit((value & 0x80) != 0);
        value <<= 1;
    }

    return !read_bit();
}

uint8_t i2c_bit_bang::read_byte(bool nack) {
    uint8_t value = 0;
    for (int i = 0; i < 8; i++) {
        value <<= 1;
        value |= read_bit();
    }

    write_bit(nack);

    return value;
}

void i2c_bit_bang::write_bit(bool bit) {
    if (bit) {
        set_sda();
    } else {
        clear_sda();
    }
    delay();
    set_scl();
    delay();
    clear_scl();
}

bool i2c_bit_bang::read_bit() {
    set_sda();
    delay();
    set_scl();
    delay();
    bool bit = read_sda();
    clear_scl();
    return bit;
}

void i2c_bit_bang::delay() {
    for (int i = 10; i > 0; i--) {
        asm("nop");
    }
}

} // namespace usb_pd
