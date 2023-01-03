//
// USB Power Delivery Sink Using FUSB302B
// Copyright (c) 2020 Manuel Bleichenbacher
//
// Licensed under MIT License
// https://opensource.org/licenses/MIT
//
// EEPROM emulation according to AN4061
//

// Flash organization:
//
// Each page consists of 4 bytes of header and 255 slots of bytes.
//
// Header:
//   page status (2 bytes)
//   unused (2 bytes)
//
// Slot:
//   key (2 bytes)
//   value (2 bytes)
//
// Keys must be in the range 0 to `num_keys` - 1.

#include "eeprom.h"
#include <libopencm3/stm32/flash.h>

namespace usb_pd {

/// Page is empty
constexpr uint16_t page_status_erased = 0xffff;
/// Page is being written to during page transfer
constexpr uint16_t page_status_in_transfer = 0xeeee;
/// Page contains valid data
constexpr uint16_t page_status_valid = 0x0000;

/// Size of the sectors (1KB)
constexpr uint32_t page_size = 0x0400;

/// EEPROM enulation start address : last 2 sectors of 16KB flash memory
constexpr uint32_t eeprom_start_addr = 0x08003800;

// Pages 0 and 1 base addresses
constexpr uint32_t page_0_base_addr = eeprom_start_addr;
constexpr uint32_t page_1_base_addr = eeprom_start_addr + page_size;

int eeprom::num_keys_ = 0;

static inline uint16_t read_uint16(uint32_t addr) {
    return *reinterpret_cast<volatile uint16_t*>(addr);
}

static inline uint32_t read_uint32(uint32_t addr) {
    return *reinterpret_cast<volatile uint32_t*>(addr);
}

void eeprom::init(int num_keys) {
    num_keys_ = num_keys;

    uint16_t page_0_status = read_uint16(page_0_base_addr);
    uint16_t page_1_status = read_uint16(page_1_base_addr);

    flash_unlock();

    // check for invalid header states and repair if necessary
    switch (page_0_status) {

    case page_status_valid:
        if (page_1_status == page_status_erased) {
            // nothing to do: page 0 is active, page 1 is empty
        } else if (page_1_status == page_status_in_transfer) {
            // redo transfer from page 0 to page 1
            uint16_t first_key = read_uint16(page_1_base_addr + 4);
            copy_slots(first_key);
            flash_erase_page(page_0_base_addr);
            flash_program_half_word(page_1_base_addr, page_status_valid);
        } else {
            format();
        }
        break;

    case page_status_erased:
        if (page_1_status == page_status_valid) {
            // nothing do: page 1 is active, page 0 is empty
        } else if (page_1_status == page_status_in_transfer) {
            // partial recovery of page 1 (in transfer)
            flash_erase_page(page_0_base_addr);
            flash_program_half_word(page_1_base_addr, page_status_valid);
        } else {
            format();
        }
        break;

    case page_status_in_transfer:
        if (page_1_status == page_status_valid) {
            // redo transfer from page 1 to page 0
            uint16_t first_key = read_uint16(page_0_base_addr + 4);
            copy_slots(first_key);
            flash_erase_page(page_1_base_addr);
            flash_program_half_word(page_0_base_addr, page_status_valid);
        } else if (page_1_status == page_status_erased) {
            // partial recovery of page 0 (in transfer)
            flash_erase_page(page_1_base_addr);
            flash_program_half_word(page_0_base_addr, page_status_valid);
        } else {
            format();
        }
        break;

    default:
        format();
        break;
    }

    flash_lock();
}

bool eeprom::read(uint16_t key, uint16_t& value) {
    uint32_t page_start_addr = find_valid_page(operation_e::read);
    if (page_start_addr == 0)
        return false;

    // Read slots from the back (the latest matching entry is the valid one)
    for (uint32_t slot = page_start_addr + page_size - 4; slot > page_start_addr; slot -= 4) {
        if (read_uint16(slot) == key) {
            value = read_uint16(slot + 2);
            return true;
        }
    }

    // Not found
    return false;
}

bool eeprom::write(uint16_t key, uint16_t value) {
    flash_unlock();

    status_e status = append_key_value(key, value);

    if (status == status_e::page_full)
        status = transfer_page(key, value);

    flash_lock();

    return status == status_e::op_completed;
}

/// Returns the address of the page valid for the specified operation
uint32_t eeprom::find_valid_page(operation_e operation) {
    uint16_t page_0_status = read_uint16(page_0_base_addr);
    uint16_t page_1_status = read_uint16(page_1_base_addr);

    switch (operation) {
    case operation_e::write:
        if (page_1_status == page_status_valid) {
            if (page_0_status == page_status_in_transfer) {
                return page_0_base_addr;
            } else {
                return page_1_base_addr;
            }
        } else if (page_0_status == page_status_valid) {
            if (page_1_status == page_status_in_transfer) {
                return page_1_base_addr;
            } else {
                return page_0_base_addr;
            }
        } else {
            return 0;
        }

    case operation_e::read:
        if (page_0_status == page_status_valid) {
            return page_0_base_addr;
        } else if (page_1_status == page_status_valid) {
            return page_1_base_addr;
        } else {
            return 0;
        }

    default:
        return page_0_base_addr;
    }
}

/**
 * Writes a key/value pair to the next free slot.
 * @param key key
 * @param value value
 * @return status (`op_completed`, `page_full`, `no_valid_page`)
 */
eeprom::status_e eeprom::append_key_value(uint16_t key, uint16_t value) {
    uint32_t page_start_addr = find_valid_page(operation_e::write);
    if (page_start_addr == 0)
        return status_e::no_valid_page;

    uint32_t page_end_address = page_start_addr + page_size;

    // Search for free slot from the start
    for (uint32_t slot = page_start_addr + 4; slot < page_end_address; slot += 4) {
        if (read_uint32(slot) == 0xFFFFFFFF) {
            // Empty slot, write key and value
            flash_program_half_word(slot, key);
            flash_program_half_word(slot + 2, value);
            return status_e::op_completed;
        }
    }

    return status_e::page_full;
}

/// Erases page 0 and 1 and marks page 0 as valid
void eeprom::format() {
    flash_erase_page(page_0_base_addr);
    flash_program_half_word(page_0_base_addr, page_status_valid);
    flash_erase_page(page_1_base_addr);
}

/**
 * Copy all key/value pairs to a new page.
 *
 * The specified key/value pair is added first and becomes the
 * new revision for the specified key.
 *
 * Only the latest value will be copied, thereby discarding earlier
 * revisions and reducing the amount of data.
 *
 * @param key key
 * @param value value
 * @return status (`op_completed`, `no_valid_page`)
 */
eeprom::status_e eeprom::transfer_page(uint16_t key, uint16_t data) {
    uint32_t page_start_addr = find_valid_page(operation_e::read);
    if (page_start_addr == 0)
        return status_e::no_valid_page;

    uint32_t old_page_address = page_start_addr;
    uint32_t new_page_address = page_start_addr == page_0_base_addr ? page_1_base_addr : page_0_base_addr;

    flash_program_half_word(new_page_address, page_status_in_transfer);

    // First add the specified key/value pair
    status_e status = append_key_value(key, data);
    if (status != status_e::op_completed)
        return status;

    // Transfer old other key/value pairs
    copy_slots(key);
    flash_erase_page(old_page_address);
    flash_program_half_word(new_page_address, page_status_valid);

    return status_e::op_completed;
}

/**
 * Copy all valid key/value pairs from the old to the new page
 * except for the one with the specified key.
 *
 * The page with status `valid` will automatically be chosen as
 * the source, and the page with status `in_transfer` as the target.
 *
 * @param skip_key key to skip
 */
void eeprom::copy_slots(uint16_t skip_key) {
    for (uint16_t k = 0; k < num_keys_; k++) {
        if (k == skip_key)
            continue;

        uint16_t value;
        if (eeprom::read(k, value))
            append_key_value(k, value);
    }
}

} // namespace usb_pd
