//
// USB Power Delivery Sink Using FUSB302B
// Copyright (c) 2020 Manuel Bleichenbacher
//
// Licensed under MIT License
// https://opensource.org/licenses/MIT
//
// Hardware abstraction layer
//

#include "hal.h"

#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#include "i2c_bit_bang.h"
#include "pd_debug.h"

namespace usb_pd {

constexpr auto fusb302_int_n_port = GPIOA;
constexpr uint16_t fusb302_int_n_pin = GPIO13;
constexpr uint8_t fusb302_i2c_addr = 0x22;

static i2c_bit_bang i2c;

static volatile uint32_t millis_count;

void mcu_hal::init()
{
    rcc_clock_setup_in_hsi_out_48mhz();

    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);

    // Initialize systick (nterrupt every 1ms)
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    systick_set_reload(rcc_ahb_frequency / 1000 - 1);
    systick_interrupt_enable();
    systick_counter_enable();

    gpio_mode_setup(fusb302_int_n_port, GPIO_MODE_INPUT, GPIO_PUPD_NONE, fusb302_int_n_pin);

    i2c.init();
}

void mcu_hal::pd_ctrl_read(uint8_t reg, int data_len, uint8_t* data)
{
    bool ack = i2c.read_data(fusb302_i2c_addr, reg, data_len, data);
    if (!ack)
        DEBUG_LOG("NACK read %d\r\n", reg);
}

void mcu_hal::pd_ctrl_write(uint8_t reg, int data_len, const uint8_t* data, bool end_with_stop)
{
    bool ack = i2c.write_data(fusb302_i2c_addr, reg, data_len, data, end_with_stop);
    if (!ack)
        DEBUG_LOG("NACK write %d\r\n", reg);
}

bool mcu_hal::is_interrupt_asserted() { return gpio_get(fusb302_int_n_port, fusb302_int_n_pin) == 0; }

uint32_t mcu_hal::millis() { return millis_count; }

void mcu_hal::delay(uint32_t ms)
{
    int32_t target_time = millis_count + ms;
    while (target_time - (int32_t)millis_count > 0)
        ;
}

bool mcu_hal::has_expired(uint32_t timeout) { return (int32_t)timeout - (int32_t)millis_count <= 0; }

} // namespace usb_pd

// System tick timer interrupt handler
extern "C" void sys_tick_handler() { usb_pd::millis_count++; }
