//
// USB Power Delivery Sink Using FUSB302B
// Copyright (c) 2020 Manuel Bleichenbacher
//
// Licensed under MIT License
// https://opensource.org/licenses/MIT
//
// SWD mode for attaching a debugger via SWD
// (Debugging is not possible but firmware upload).
//

#include "swd.h"

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/gpio.h>

#include "pd_debug.h"

namespace usb_pd {

constexpr auto swdio_port = GPIOA;
constexpr uint16_t swdio_pin = GPIO13;
constexpr auto swclk_port = GPIOA;
constexpr uint16_t swclk_pin = GPIO14;
constexpr uint32_t swclk_exti = EXTI14;

bool swd::activity_detected_ = false;
stop_f swd::stop_f_ = nullptr;

void swd::init_monitoring(stop_f stop)
{
    stop_f_ = stop;

    // Configure SWCLK as input with external interup.
    // If activity is detected, the SWD is restored
    gpio_mode_setup(swclk_port, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, swclk_pin);

    nvic_set_priority(NVIC_EXTI4_15_IRQ, 2 << 6);
    nvic_enable_irq(NVIC_EXTI4_15_IRQ);

    exti_select_source(swclk_exti, swclk_port);
    exti_set_trigger(swclk_exti, EXTI_TRIGGER_FALLING);
    exti_enable_request(swclk_exti);
}

void swd::restore()
{
    stop_f_();

    // restore SWD
    gpio_set_af(swclk_port, GPIO_AF0, swclk_pin);
    gpio_mode_setup(swclk_port, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, swclk_pin);
    gpio_set_af(swdio_port, GPIO_AF0, swdio_pin);
    gpio_mode_setup(swdio_port, GPIO_MODE_AF, GPIO_PUPD_PULLUP, swdio_pin);

    DEBUG_LOG("Firmware mode\r\n", 0);
}

} // namespace usb_pd

extern "C" void exti4_15_isr()
{
    // SWCLK activity has been detected
    exti_disable_request(usb_pd::swclk_exti);
    exti_reset_request(usb_pd::swclk_exti);
    usb_pd::swd::activity_detected_ = true;
}
