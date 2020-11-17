//
// USB Power Delivery Sink Using FUSB302B
// Copyright (c) 2020 Manuel Bleichenbacher
//
// Licensed under MIT License
// https://opensource.org/licenses/MIT
//

#include <libopencm3/stm32/gpio.h>

#include "hal.h"
#include "pd_debug.h"
#include "pd_sink.h"
#include "swd.h"

using namespace usb_pd;

constexpr auto led_red_port = GPIOA;
constexpr uint16_t led_red_pin = GPIO5;
constexpr auto led_green_port = GPIOA;
constexpr uint16_t led_green_pin = GPIO6;
constexpr auto led_blue_port = GPIOA;
constexpr uint16_t led_blue_pin = GPIO7;

mcu_hal usb_pd::hal;

pd_sink power_sink;

void attach_state_changed();
void source_caps_changed();
void loop();
void test_for_debugger();
void firmware_loop();

static uint32_t last_blink = hal.millis();

int main()
{
    hal.init();
    DEBUG_INIT();

    gpio_mode_setup(led_red_port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, led_red_pin);
    gpio_set(led_red_port, led_red_pin);
    gpio_mode_setup(led_green_port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, led_green_pin);
    gpio_set(led_green_port, led_green_pin);
    gpio_mode_setup(led_blue_port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, led_blue_pin);
    gpio_set(led_blue_port, led_blue_pin);

    swd::init_monitoring(power_sink);

    power_sink.set_attach_state_changed_callback(attach_state_changed);
    power_sink.set_source_caps_changed_callback(source_caps_changed);
    power_sink.init();

    while (!swd::activity_detected())
        loop();
    firmware_loop();
    return 0;
}

void loop()
{
    uint32_t diff = hal.millis() - last_blink;
    if (diff > 500) {
        gpio_toggle(led_blue_port, led_blue_pin);
        last_blink += 500;
    }

    power_sink.poll();
}

void firmware_loop()
{
    // Restore SWD pins so firmware can be uploaded
    swd::restore();

    // Fast blinking red color
    while (true) {
        gpio_toggle(led_red_port, led_red_pin);
        hal.delay(100);
    }
}

void attach_state_changed()
{
#if defined(PD_DEBUG)
    int index = static_cast<int>(power_sink.attach_state());
    const char* const state_names[] = { "disabled", "unattached", "attached" };
#endif

    DEBUG_LOG("New state: ", 0);
    DEBUG_LOG(state_names[index], 0);
    DEBUG_LOG("\r\n", 0);
}

void source_caps_changed()
{
    DEBUG_LOG("Caps changed\r\n", 0);

    int voltage = 5000;
    int current = 500;

    // If available, select 9V at 1500mA
    for (int i = 0; i < power_sink.num_source_caps; i++) {
        if (power_sink.source_caps[i].voltage == 9000 && power_sink.source_caps[i].max_current >= 1500) {
            voltage = 9000;
            current = 1500;
        }
    }

    power_sink.request_power(voltage, current);
}
