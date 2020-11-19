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

uint8_t selected_cap = 0;

void sink_callback(callback_event event);
void update_led();
void switch_voltage();
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

    if (hal.has_button_been_pressed())
        switch_voltage();
}

void switch_voltage()
{
    if (power_sink.protocol() != pd_protocol::usb_pd)
        return;

    while (true) {
        selected_cap++;
        if (selected_cap >= power_sink.num_source_caps)
            selected_cap = 0;
        if (power_sink.source_caps[selected_cap].supply_type != pd_supply_type::fixed)
            continue;
        power_sink.request_power(
            power_sink.source_caps[selected_cap].voltage, power_sink.source_caps[selected_cap].max_current);
        break;
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

    switch (event) {
    case callback_event::source_caps_changed:
        DEBUG_LOG("Caps changed\r\n", 0);
        selected_cap = 0;
        power_sink.request_power(power_sink.source_caps[0].voltage, power_sink.source_caps[0].max_current);
        break;

    case callback_event::power_ready:
        DEBUG_LOG("Voltage: %d\r\n", power_sink.active_voltage);
        break;

    case callback_event::protocol_changed:
        if (power_sink.protocol() == pd_protocol::usb_20)
            selected_cap = 0;
        break;

    default:
        break;
    }

    update_led();
}

void update_led()
{
    if (power_sink.protocol() == pd_protocol::usb_20) {
        hal.set_led(color::blue, 800, 600);
        return;
    }

    color c = color::red;
    int blink = 0;
    switch (power_sink.active_voltage) {
    case 5000:
        c = color::red;
        break;
    case 9000:
        c = color::yellow;
        break;
    case 12000:
        c = color::green;
        break;
    case 15000:
        c = color::cyan;
        break;
    case 20000:
        c = color::blue;
        break;
    default:
        blink = 200;
    }

    hal.set_led(c, blink, blink);
}