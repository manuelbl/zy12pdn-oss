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
};

extern mcu_hal hal;

} // namespace usb_pd

#endif
