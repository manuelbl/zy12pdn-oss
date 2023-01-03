//
// USB Power Delivery Sink Using FUSB302B
// Copyright (c) 2020 Manuel Bleichenbacher
//
// Licensed under MIT License
// https://opensource.org/licenses/MIT
//
// Debugging support (sending debugging messages to output)
//

#pragma once

#include <stdint.h>

#if defined(PD_DEBUG)

#define DEBUG_LOG(MSG, VAL) ::usb_pd::debug_log(MSG, VAL)

#define DEBUG_INIT() ::usb_pd::debug_init()

namespace usb_pd {

void debug_log(const char* msg, uint32_t val);
void debug_init();

} // namespace usb_pd

#else

#define DEBUG_LOG(MSG, VAL) \
    do { } while (false)
#define DEBUG_INIT() \
    do { } while (false)

#endif

