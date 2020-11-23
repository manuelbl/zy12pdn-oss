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

#ifndef _swd_h_
#define _swd_h_

extern "C" void exti4_15_isr();

namespace usb_pd {

typedef void (*stop_f)();

struct swd {
    /// Setup monitoring for SWCLK activity
    static void init_monitoring(stop_f stop);

    /// Restore SWD pins
    static void restore();

    /// Returns if SWD activity has been detected
    static bool activity_detected() { return activity_detected_; }

private:
    static bool activity_detected_;
    static stop_f stop_f_;

    friend void ::exti4_15_isr();
};

} // namespace usb_pd

#endif
