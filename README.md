# swx - E-Stim Driver Firmware

swx is an E-Stim driver for the RP2040 microcontroller (e.g. Raspberry Pi Pico). Taking advantage of the PIO interface, it handles the low-level control and pulse generation of the output channels. 
Control of the driver is done via SPI or UART, direct pulses can be sent or parameters changed. Such as frequency, pulse width, power level, etc.

# Hardware

swx assumes that each E-Stim channel has the following GPIO:

- Pulse A/B - Two digital I/O pins used to create the actual E-Stim pulse.
- Analog Out\* - Used to set the power level of the channel.
- Analog In\* - Used to measure channel current, allows for analog out to be calibrated.

_\* Hardware abstracted (e.g. I2C ADC or internal RP2040 ADC)_

View the [SW32](https://github.com/saawsm/SW32) project for example hardware.
