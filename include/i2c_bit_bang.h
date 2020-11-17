// USB Power Delivery Sink Using FUSB302B
// Copyright (c) 2020 Manuel Bleichenbacher
//
// Licensed under MIT License
// https://opensource.org/licenses/MIT
//
// I2C bit banging
//

#ifndef _i2c_bit_bang_h_
#define _i2c_bit_bang_h_

#include <libopencm3/stm32/gpio.h>

namespace usb_pd {

constexpr auto scl_port = GPIOA;
constexpr uint16_t scl_pin = GPIO10;
constexpr auto sda_port = GPIOA;
constexpr uint16_t sda_pin = GPIO9;

struct i2c_bit_bang {
    void init();

    bool write_data(uint8_t addr, uint8_t reg, int data_len, const uint8_t* data, bool end_with_stop = true);
    bool read_data(uint8_t addr, uint8_t reg, int data_len, uint8_t* data);

    void write_start_cond();
    void write_stop_cond();
    bool write_byte(uint8_t value);
    uint8_t read_byte(bool nack);

    void write_bit(bool bit);
    bool read_bit();

    void delay();
    void set_scl() { gpio_set(scl_port, scl_pin); }
    void clear_scl() { gpio_clear(scl_port, scl_pin); }
    void set_sda() { gpio_set(sda_port, sda_pin); }
    void clear_sda() { gpio_clear(sda_port, sda_pin); }
    bool read_sda() { return gpio_get(sda_port, sda_pin) != 0; }

    bool is_started = false;
};

} // namespace usb_pd

#endif
