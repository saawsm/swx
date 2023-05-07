# swx - E-Stim Driver Firmware

swx is an E-Stim driver for the RP2040 microcontroller (e.g. Raspberry Pi Pico). Taking advantage of the PIO interface, it handles the low-level control and pulse generation of the output channels.
Control of the driver is done via SPI or UART, direct pulses can be sent, or parameters changed. Such as frequency, pulse width, power level, etc.

# Hardware Requirements

swx assumes that each E-Stim channel has the following IO:

- Pulse A/B - Two digital GPIO pins used to create the actual E-Stim pulse.
- Level<sup>1</sup> - Used to set the output power of the channel.
- Sense<sup>1,2</sup> - Used to measure channel current, allowing for calibration and real-time fault monitoring.

_\*1: Hardware abstracted_<br/>
_\*2: Supports sharing the same IO between channels_

View the [SW32](https://github.com/saawsm/SW32) or [SW22](https://github.com/saawsm/SW22) projects for example hardware.

