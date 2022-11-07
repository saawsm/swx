## swx - E-Stim Driver Firmware

swx is an E-Stim driver for the RP2040 microcontroller (Raspberry Pi Pico). It handles the low-level control and routine/pattern generation of the output channel modules. This allows for high level commands to be used via a communication method such as SPI or I2C.

## Hardware

swx assumes that each E-Stim channel has the following 3.3V GPIO:

- Pulse A/B - Two digital I/O pins used to create the actual E-Stim pulse.
- Analog Out - Used to set the power level of the channel.
- Analog In - Used to measure channel current, so analog out can be calibrated.

View the [SW32](https://github.com/saawsm/SW32) project for example hardware.