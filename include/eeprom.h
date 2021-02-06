//
// USB Power Delivery Sink Using FUSB302B
// Copyright (c) 2020 Manuel Bleichenbacher
//
// Licensed under MIT License
// https://opensource.org/licenses/MIT
//
// EEPROM emulation according to AN4061
//

#ifndef _eeprom_h_
#define _eeprom_h_

#include <stdint.h>

namespace usb_pd {

/**
 * EEPROM emulation.
 *
 * Values can be stored at key 0 to `num_keys` - 1.
 * `num_keys` must be set at initialization and should
 * be chosen as small as possible.
 */
struct eeprom {

    /**
     * Initializes the EEPROM (at startup).
     * 
     * Restores the pages to a known good state if needed.
     * 
     * @param num_keys number of keys (0 to `num_keys` - 1).
     */
    static void init(int num_keys);

    /**
     * Reads the 16 bit value from the specified key.
     * 
     * @param key key
     * @param value reference to variable receiving value
     * @return `true` if value was found, `false` otherwise
     */
    static bool read(uint16_t key, uint16_t* value);

    /**
     * Writes a 16 bit value for the specified key.
     * 
     * @param key key
     * @param value value to write
     * @return `true` if the write was successful, `false` otherwise
     */
    static bool write(uint16_t key, uint16_t value);

    static int num_keys_;

private:
    enum class status_e { no_valid_page, page_full, op_completed };

    enum class operation_e { read, write };

    static void format();
    static status_e append_key_value(uint16_t key, uint16_t value);
    static status_e transfer_page(uint16_t key, uint16_t value);
    static uint32_t find_valid_page(operation_e operation);
    static void copy_slots(uint16_t skip_key);
};

} // namespace usb_pd

#endif
