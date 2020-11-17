# Open-Source Firmware for ZY12PDN USB Power Delivery Trigger

![ZY12PDN board](doc/board.jpg)

## Building

- Clone the project from GitHub
- Open it with Visual Studio Code
- Install the PlatformIO extension
- Click the build icon in the status bar

## Upload

The ZY12PDN board has a 4-pin SWD pads at the bottom. Either solder wires to them or use a 4-pin adapter with pogo pins.

![SWD](doc/swd.jpg)

Connect the SWD pads with an ST-Link, J-Link or Black Magic Probe to your computer and click the upload icon in the status bar of Visual Studio Code.

In most cases, you will need to try twice since the board does not enable the SWD pins quickly enough.

## Notes

- So far, the only supported messages are *Capabilities* (the source announcing the supported voltages) and *Request* (the sink requesting a specific voltage). More to come soon.
- In the `source_caps_changed` callback, `request_power` must be called to request a voltage -- even if it is 5V. Otherwise the source is likely to reset.
- VBUS is directly connected from the USB-C socket to VBUS (+) of the output. So for the board to be more useful, it would probably make sense to add a MOS-FET as a switch further downstream so that power is only turned on once the correct voltage is available. Initially, VBUS will always start with 5V. To control the MOSFET, you have to solder a wire to one of the unused MCU pins.
- The firmware is currently limited to the fixed voltages. Additionally capabilities (variable voltages etc.) can be easily added.
- The firmware does not properly work with Apple's 87W USB-C Power Adapter. It eventually works but the voltage is cut for a short time and reapplied. The firmware reboots and after that is occupied with endless interrupts. 
- Using the build flag `-D PD_DEBUG`, debugging output can be enabled. In order to see it, you have to solder a wire to PA2 (USART2 TX pin) and connect it to a serial adapter. The baud rate is 115,200 bps.
- All the code is very timing sensitive. Be very careful with debugging output in the `source_caps_changed` callback. It the debugging output takes too long, the USB power supply will likely reset and even cut the power.
