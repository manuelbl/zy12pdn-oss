# Open-Source Firmware for ZY12PDN USB-PD

Open-source firmware for USB Power Delivery trigger board based on an FUSB302B power delivery controller and a STM32F030F4 MCU.

![ZY12PDN board](doc/board.jpg)


## Building

- Clone the project from GitHub
- Open it with Visual Studio Code
- Install the PlatformIO extension
- Click the build icon in the status bar


## Upload

The ZY12PDN board has 4 pads for SWD at the bottom. Either solder wires to them, or use a 4-pin adapter with pogo pins to connect a debug adapter.

![SWD](doc/swd.jpg)

Connect the SWD pads with an ST-Link, J-Link or Black Magic Probe to your computer and click the upload icon in the status bar of Visual Studio Code.

As sold, the boards have flash protection enabled. It can be disabled using *openocd*. Note: This command erases the flash memory:

```
openocd -f interface/stlink.cfg -f target/stm32f0x.cfg -c "init; reset halt; stm32f0x unlock 0; stm32f0x mass_erase 0; reset halt; exit"
```


## Hardware

See [Hardware](doc/hardware.md) for a detailed description of the board and its components (incl. schematic).


## Usage Instructions

The user interface – if it can be called so – is similar to the original ZY12PDN. The board can work in one of three modes:

- **Interactive mode**: By pressing the button, the board switches between available voltages.
- **Fixed voltage mode**: The board provides a configured voltage. If the configured voltage is not available, the board will output 5V.
- **Configuration mode**: By pressing the button, either the interactive mode or one of the fixed voltage modes is selected.

The LED color indicates either the active voltage (interactive and fixed voltage mode) or the desired voltage (configuration mode).


| Color  | Interactive | Fixed Voltage | Configuration |
| :----- | :-- | :-- | :-- |
| Red    | 5V  | Configured voltage is not available † | Configure interactive mode. |
| Yellow | 9V  | 9V  | Configure fixed voltage mode with 9V. |
| Green  | 12V | 12V | Configure fixed voltage mode with 12V. |
| Cyan   | 15V | 15V | Configure fixed voltage mode with 15V. |
| Blue   | 20V | 20V | Configure fixed voltage mode with 20V. |
| Purple | –   | -   | Configure fixed voltage mode with highest available voltage. |


*† The LED will also blink in red if the board has been configured for the highest available voltage and the highest available voltage is neither 9V, 12V, 15V nor 20V.*

### Configuring the Board

The configuration mode is entered by plugging in the board while pressing the button. The LED will flash quickly in cyan until the button is released. The mode can be selected by pressing the button. To save the configuration, the button must be pressed for a longer period until the LED goes off. During configuration, the output voltage remains at 5V.


## Supported PD Messages

 - *Capabilities*: The source announces the supported voltages (called "capabilities"). The sink must immediately request one of them.
 - *Request*: The sink requests a specific voltage and maximum current.
 - *Accept*: The source confirms the requested voltage. The new voltage is not ready yet.
 - *Reject*: The source rejects the requested voltage.
 - *PS_RDY*: The source indicates that the requested voltage has been applied.


## Notes

- If the event type of the `pd_sink` callback is `callback_event::source_caps_changed`, `request_power()` must be called to request a voltage – even if it is 5V. Otherwise the source will reset.
- The firmware is currently limited to the fixed voltages, i.e. pressing the button switches between the fixed voltages. The class `pd_sink` also support *programmable power supply* (PPS) capabilities. But a different user interfaces / modified firmware is needed to actually use them.
- Using the build flag `-D PD_DEBUG`, debugging output can be enabled. In order to see it, you have to solder a wire to PA2 (USART2 TX pin) and connect it to a serial adapter. The baud rate is 115,200 bps (8 bits, 1 stop bit, no parity).
- Since the USB PD protocol is timing sensitive, debug output must not delay normal operation. For that reason, the debug output is written to a circular buffer and then transmitted using non-blocking DMA. Blocking UART operation would introduce delays and violate the timing causing the power supply to reset or even to cut power.


## Firmware Mode

The SWDIO line is shared with the interrupt line of the USB PD controller.

In order to enable firmware upload, the FUSB302B interrupt is not activated until USB PD activity has been detected. To implement this behavior, the code manually switches between measuring CC1 and CC2 even though the FUSB302B chip could do it automatically.

To upload firmware, connect a debug adapter (such as ST-Link or J-Link) to the SWD pads on the bottom of the board and provide power from any source except a USB PD power supply:

- Feed power to the USB connector from a non USB PD power supply
- Feed 3.3V to the VCC pad on the bottom side of the board (e.g. through an older ST-Link adapter)
- Feed 5V to the output terminal

SWD can be used to upload new firmware. But debugging is only possible until the interrupt pin is activated in code.


## Acknowledgements

Thanks to the people that have also analyzed the ZY12PDN board and contributed to this work:

- Alex Whittemore: [Notes on USB PD Triggers (and ZY12PDN Instructions)](https://www.alexwhittemore.com/notes-on-usb-pd-triggers-and-zy12pdn-instructions/) and [ZY12PDN Reverse Engineering Part 1](https://www.alexwhittemore.com/zy12pdn-reverse-engineering-part-1/).
- Brian Lough: [Powering your projects uing USB-C Power Delivery (YouTube)](https://www.youtube.com/watch?v=iumAnPiQSj8)
- *OxPeter* and *MarkOlsson* and further people on [Brian Lough's Discord Channel](https://discord.gg/nnezpvq)
