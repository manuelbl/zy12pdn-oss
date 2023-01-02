//
// USB Power Delivery Sink Using FUSB302B
// Copyright (c) 2020 Manuel Bleichenbacher
//
// Licensed under MIT License
// https://opensource.org/licenses/MIT
//
// Hardware abstraction layer
//

#ifndef _hal_h_
#define _hal_h_

#include <stdint.h>

namespace usb_pd {

enum class color {
    white = 0b000,
    yellow = 0b001,
    purple = 0b010,
    red = 0b011,
    cyan = 0b100,
    green = 0b101,
    blue = 0b110,
    off = 0b111
};

inline uint8_t operator*(color c) { return static_cast<uint8_t>(c); }

/**
 * Hardware abstraction layer.
 *
 * Provides required services:
 *   - I2C communication with PD controller
 *   - time
 */
struct mcu_hal {
    /**
     * Initializes the hardware abstraction layer.
     * Needs to be called once when the MCU starts.
     */
    void init();

    /**
     * Configures the INT_N pin as input (disabling SWD)
     */
    void init_int_n();

    /**
     * Read data from PD controller registers.
     *
     * @param reg register address
     * @param data_len length of data to read (number of bytes)
     * @param data buffer for read data
     */
    void pd_ctrl_read(uint8_t reg, int data_len, uint8_t* data);

    /**
     * Write data to PD controller registers.
     *
     * @param reg register address
     * @param data_len length of data to write (number of bytes)
     * @param data buffer with data to be written
     * @param end_with_stop indicates if the I2C transaction should end with a STOP condition
     */
    void pd_ctrl_write(uint8_t reg, int data_len, const uint8_t* data, bool end_with_stop = true);

    /**
     * Gets if the interrupt pin is assert (low).
     *
     * @return `true` if the pin is asserted, `false` otherwise.
     */
    bool is_interrupt_asserted();

    /**
     * Sets the LED color and flash pattern.
     *
     * If the LED is not supposed to flash, specify 0 for both
     * 'on' and 'off'.
     *
     * @param c color
     * @param on flash on duration (in ms)
     * @param off flash off duration (in ms)
     */
    void set_led(color c, uint32_t on = 0, uint32_t off = 0);

    /**
     * Returns if the button has been pressed.
     * 
     * Button presses are reported after the button has
     * been released.
     * 
     * It will return 'true' once and then return 'false' until
     * the button has been released and pressed again.
     * 
     * @return `true` if the button has been pressed
     */
    bool has_button_been_pressed();

    /**
     * Returns true if the button is currently being pressed.
     * 
     * @return `true`  if the button is being pressed
     */
    bool is_button_being_pressed();

    /**
     * Returns if the button is being pressed for an extended period.
     * 
     * Use to check for long press.
     * 
     * @return `true` if the button is being pressed.
     */
    bool is_long_press();

    /**
     * Call this function frequently to update the LED state
     * and handle button presses.
     */
    void poll();

    /**
     * Returns time stamp.
     *
     * @return number of milliseconds since a fixed time in the past.
     */
    uint32_t millis();

    /**
     * Sleep for the specified time
     *
     * @param ms time to sleep (in milliseconds).
     */
    void delay(uint32_t ms);

    /**
     * Returns if the specified timeout time has been reached or passed.
     * @param timeout timeout time
     * @return 'true' if it has been reached or passed, 'false' otherwise
     */
    bool has_expired(uint32_t timeout);

private:
    void update_led();

    color led_color;
    uint32_t led_on;
    uint32_t led_off;
    bool is_led_on;
    uint32_t led_timeout;
    uint32_t last_button_change_time;
    bool is_button_down;
    bool button_has_been_pressed;
};

extern mcu_hal hal;

} // namespace usb_pd

#endif
