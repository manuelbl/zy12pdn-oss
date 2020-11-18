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

mcu_hal usb_pd::hal;

pd_sink power_sink;

uint8_t col = 6;

void sink_callback(callback_event event);
void source_caps_changed();
void loop();
void test_for_debugger();
void firmware_loop();

int main()
{
    hal.init();
    hal.set_led(color::blue, 800, 600);
    DEBUG_INIT();

    swd::init_monitoring(power_sink);

    power_sink.set_event_callback(sink_callback);
    power_sink.init();

    while (!swd::activity_detected())
        loop();

    firmware_loop();
    return 0;
}

void loop()
{
    hal.poll();
    power_sink.poll();

    if (hal.has_button_been_pressed()) {
        col++;
        if (col >= 7)
            col = 1;
        hal.set_led(static_cast<color>(col));
    }
}

void firmware_loop()
{
    hal.set_led(color::red, 100, 100);

    // Restore SWD pins so firmware can be uploaded
    swd::restore();

    // Fast blinking red color
    while (true) {
        hal.poll();
    }
}

void sink_callback(callback_event event)
{
#if defined(PD_DEBUG)
    int index = static_cast<int>(event);
    const char* const event_names[]
        = { "protocol_changed", "source_caps_changed", "power_accepted", "power_rejected", "power_ready" };

    DEBUG_LOG("Event: ", 0);
    DEBUG_LOG(event_names[index], 0);
    DEBUG_LOG("\r\n", 0);
#endif

    if (event == callback_event::source_caps_changed)
        source_caps_changed();

    if (event == callback_event::power_ready) {
        DEBUG_LOG("Voltage: %d\r\n", power_sink.active_voltage);
    }
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
