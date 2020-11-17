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
