# Open-Source Firmware for ZY12PDN USB-PD

Open-source firmware for USB Power Delivery trigger board based on an FUSB302B power delivery controller and a STM32F030F4 MCU.

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


## Hardware

See [Hardware](doc/hardware.md) for a detailled description of the board and its components (incl. schematic).


## Supported PD Messages

 - *Capabilities*: The source announces the supported voltages. The sink must immediately request one of the voltages.
 - *Request*: The sink requests a specific voltage and maximum current.
 - *Accept*: The source confirms the requested voltage. The new voltage is not ready yet.
 - *Reject*: The source rejects the requested voltage.
 - *PS_RDY*: The source indicates that the request voltage has been applied.


## Notes

- If the event type of the `pd_sink` callback is `callback_event::source_caps_changed`, `request_power()` must be called to request a voltage -- even if it is 5V. Otherwise the source is likely to reset.
- The firmware is currently limited to the fixed voltages. Additionally capabilities (variable voltages etc.) can be easily added.
- Using the build flag `-D PD_DEBUG`, debugging output can be enabled. In order to see it, you have to solder a wire to PA2 (USART2 TX pin) and connect it to a serial adapter. The baud rate is 115,200 bps.
- All the code is very timing sensitive. Be very careful with debugging output in the `source_caps_changed` callback. It the debugging output takes too long, the USB power supply will likely reset and even cut the power.


## Firmware Mode

The SWDIO line is shared with the interrupt line of the USB PD controller. Therefore, uploading firmware is tricky.

The firmware needs to decide if a debugger is connected or not:

- If a debugger is connected, the FUSB302B is turned off so it releases the SWDIO pin and the SWD has to be enabled.
- If no debugger is connected, the FUSB302B is configured, and it uses the SWDIO to signal interrupts.

Currently, SWCLK is initially configured as input with an external interrupt. If activity is detected, FSUSB302B is shut down and the SWCLK and SWDIO are restored for SWD operation. This has two disadvantages: it can be inadvertently triggered by touching the SWCLK pad on the bottom of the board, and swtich to firmware mode is not always fast enough so uploading firmware takes two attemps.

If you know of a better approach to detect a debugger, let me know.

SWD can be used to upload new firmware. But debugging is not possible as – in normal operation – the SWDIO pin is used as the interrupt pin.


## Acknowledgements

Thanks to the people that have also analzed the ZY12PDN board and contributed to this work:

- Alex Whittemore: [Notes on USB PD Triggers (and ZY12PDN Instructions)](https://www.alexwhittemore.com/notes-on-usb-pd-triggers-and-zy12pdn-instructions/) and [ZY12PDN Reverse Engineering Part 1](https://www.alexwhittemore.com/zy12pdn-reverse-engineering-part-1/).
- Brian Lough: [Powering your projects uing USB-C Power Delivery (YouTube)](https://www.youtube.com/watch?v=iumAnPiQSj8)
- *OxPeter* and *MarkOlsson* on further people on [Brian Lough's Discord Channel](https://discord.gg/nnezpvq)
