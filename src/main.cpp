//
// USB Power Delivery Sink Using FUSB302B
// Copyright (c) 2020 Manuel Bleichenbacher
//
// Licensed under MIT License
// https://opensource.org/licenses/MIT
//

#include "eeprom.h"
#include "pd_debug.h"
#include "pd_sink.h"

#include <algorithm>

using namespace usb_pd;

constexpr uint16_t nvs_voltage_key = 0;

mcu_hal usb_pd::hal;

static pd_sink power_sink;

static eeprom nvs;

static uint8_t selected_capability = 0;

// Modes:
// 0: voltage selectable by button
// 100: maximum voltage
// others: voltage in V
static uint16_t desired_mode = 0;

static bool in_config_mode = false;

static void sink_callback(callback_event event);
static void update_led();
static void switch_voltage();
static void on_source_caps_changed();
static void loop();
static void run_config_mode();
static void set_led_prog_mode(int mode);
static void save_mode(int mode);
static int mode_to_voltage(int mode);
static int voltage_to_mode(int voltage);

int main() {
    hal.init();
    hal.set_led(color::blue, 800, 600);
    nvs.init(3);

    // Read the configured mode
    if (!nvs.read(nvs_voltage_key, desired_mode))
        desired_mode = 0;

    DEBUG_LOG("Saved mode: %d\r\n", desired_mode);

    power_sink.set_event_callback(sink_callback);
    power_sink.init();

    // Wait 60ms for button presses
    uint32_t start = hal.millis();
    while (hal.millis() - start < 60) {
        hal.poll();

        // Enter configuration mode if button is being pressed on power up
        if (hal.is_button_being_pressed())
            run_config_mode();
    }

    update_led();

    // Work in regular loop
    while (true) {
        loop();

        // sleep until something happens
        hal.wait_for_event();
    }
}

// Regular operations loop
void loop() {
    hal.poll();
    power_sink.poll();

    // In mode 0, the button switches the voltage
    if (desired_mode == 0 && hal.has_button_been_pressed())
        switch_voltage();
}

// Change the voltage to the next source capability
void switch_voltage() {
    // Only works with USB PD
    if (power_sink.protocol() != pd_protocol::usb_pd)
        return;

    while (true) {
        selected_capability++;
        if (selected_capability >= power_sink.num_source_caps)
            selected_capability = 0;

        // Skip all source capabilities except fixed sources
        if (power_sink.source_caps[selected_capability].supply_type != pd_supply_type::fixed)
            continue;

        // Request the new source capability
        power_sink.request_power(power_sink.source_caps[selected_capability].voltage);
        break;
    }
}

// Called when the USB PD controller triggers an event
void sink_callback(callback_event event) {
#if defined(PD_DEBUG)
    int index = static_cast<int>(event);
    const char* const event_names[] = {"protocol_changed", "source_caps_changed", "power_accepted", "power_rejected",
                                       "power_ready"};

    DEBUG_LOG("Event: ", 0);
    DEBUG_LOG(event_names[index], 0);
    DEBUG_LOG("\r\n", 0);
#endif

    switch (event) {
    case callback_event::source_caps_changed:
        DEBUG_LOG("Caps changed: %d\r\n", power_sink.num_source_caps);
        on_source_caps_changed();
        break;

    case callback_event::power_ready:
        DEBUG_LOG("Voltage: %d\r\n", power_sink.active_voltage);
        break;

    case callback_event::protocol_changed:
        if (power_sink.protocol() == pd_protocol::usb_20)
            selected_capability = 0;
        break;

    default:
        break;
    }

    if (!in_config_mode)
        update_led();
}

// Called when the source advertises new capabilities
// Be careful with debug output. If one of the capbilities is not
// requested in time, the power suplly will reset.
void on_source_caps_changed() {
    // default: 5V
    int voltage = 5000;

    if (in_config_mode) {
        // Use default of 5V

    } else if (desired_mode == 0) {
        // Use first advertised voltage
        selected_capability = 0;
        voltage = power_sink.source_caps[0].voltage;

    } else if (desired_mode == 100) {
        // Take maximum voltage
        for (int i = 0; i < power_sink.num_source_caps; i++)
            voltage = std::max(voltage, static_cast<int>(power_sink.source_caps[i].voltage));

        // Limit voltage to 20V as the voltage regulator was likely selected to handle 20V max
        if (voltage > 20000)
            voltage = 20000;

    } else {
        // Search for desried voltage
        for (int i = 0; i < power_sink.num_source_caps; i++) {
            if (power_sink.source_caps[i].min_voltage <= desired_mode * 1000
                && power_sink.source_caps[i].voltage >= desired_mode * 1000)
                voltage = desired_mode * 1000;
        }
    }

    power_sink.request_power(voltage);
}

void update_led() {
    // LED colors indicates voltage
    color c = color::red;
    int flash_duration = 0;
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
        flash_duration = 200;
    }

    // In USB 2.0 mode or if the voltage is different from the desired one, flash the LED
    if (power_sink.protocol() == pd_protocol::usb_20) {
        flash_duration = 600;
    } else if (desired_mode != 0 && desired_mode != 100 && power_sink.active_voltage != 1000 * desired_mode) {
        flash_duration = 1000;
    }

    hal.set_led(c, flash_duration, flash_duration);
}

void run_config_mode() {
    in_config_mode = true;
    hal.set_led(color::cyan, 70, 70);

    // wait until button has been released
    while (hal.is_button_being_pressed()) {
        hal.poll();
        power_sink.poll();
    }

    // if it wasn't a proper press (50ms or more), return to regular mode
    if (!hal.has_button_been_pressed()) {
        in_config_mode = false;
        return;
    }

    DEBUG_LOG("Configuration mode\r\n", 0);

    int mode = voltage_to_mode(desired_mode);

    set_led_prog_mode(mode);

    while (true) {
        hal.poll();
        power_sink.poll();

        if (hal.has_button_been_pressed()) {
            // Button has been pressed and released -> switch to next mode
            mode++;
            if (mode > 5)
                mode = 0;
            set_led_prog_mode(mode);

        } else if (hal.is_long_press()) {
            // Button has been pressed for a long time -> save selected voltage
            save_mode(mode); // will not return
        }
    }
}

// Set LED color for selected mode
void set_led_prog_mode(int mode) {
    const color colors[] = {color::red, color::yellow, color::green, color::cyan, color::blue, color::purple};
    hal.set_led(colors[mode], 80, 40);
}

static const uint16_t voltages[] = {0, 9, 12, 15, 20, 100};

int mode_to_voltage(int mode) {
    return voltages[mode];
}

int voltage_to_mode(int voltage) {
    for (int i = 0; i < static_cast<int>(sizeof(voltages) / sizeof(voltages[0])); i++) {
        if (voltages[i] == voltage)
            return i;
    }
    return 0;
}

// Save selected mode in non-volatile storage
void save_mode(int mode) {
    nvs.write(nvs_voltage_key, mode_to_voltage(mode));
    hal.set_led(color::off);

    while (true)
        ; // end of program
}
