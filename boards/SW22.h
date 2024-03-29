#ifndef _SW22_H
#define _SW22_H

#include <boards/pico.h>

// For board detection
#define SW22

// -------- PINS --------
#define PIN_REG_EN (14)

#define PIN_LDAC (4)
#define PIN_INT (5)

#define PIN_AUDIO_MIC (26)
#define PIN_AUDIO_LEFT (27)
#define PIN_AUDIO_RIGHT (28)

#define PIN_I2S_BCLK (20)
#define PIN_I2S_WS (21)
#define PIN_I2S_SD (22)

// Triggers are unavailable for the SW22 board
#define TRIGGER_COUNT (0)
//#define PIN_TRIGGER1 (14)
//#define PIN_TRIGGER2 (15)
//#define PIN_TRIGGER3 (16)
//#define PIN_TRIGGER4 (17)

// -------- I2C --------

// Port used for communications with controller (I2C slave mode).
#define I2C_PORT_COMMS (i2c0)
#define I2C_FREQ_COMMS (100000)
#define I2C_ADDRESS_COMMS (0x17)

#define PIN_I2C_SDA_COMMS (0)
#define PIN_I2C_SCL_COMMS (1)

// Port used for DAC and any other peripheral devices (I2C master mode).
#define I2C_PORT_PERIF (i2c1)
#define I2C_FREQ_PERIF (400000)

#define PIN_I2C_SDA_PERIF (2)
#define PIN_I2C_SCL_PERIF (3)

// -------- I2C Devices --------
#define USE_ADC_ADS1015 // Use I2C ADC for feedback
#define ADC_ADDRESS (0x48)

#define USE_DAC_MCP4728 // Use I2C DAC for level control
#define DAC_ADDRESS (0x60)

// -------- Channels --------
#define CHANNEL_COUNT (4)

// -------- Channel Defaults --------
#define CH_CAL_THRESHOLD_OK (0.15f)
#define CH_CAL_THRESHOLD_OVER (0.18f)
#define CH_CAL_OFFSET (400)

// -------- Channel 1 --------
#define PIN_CH1_GA (12)
#define PIN_CH1_GB (13)
#define CH1_DAC_CHANNEL (0)
#define CH1_ADC_CHANNEL (0)

// -------- Channel 2 --------
#define PIN_CH2_GA (10)
#define PIN_CH2_GB (11)
#define CH2_DAC_CHANNEL (1)
#define CH2_ADC_CHANNEL (1)

// -------- Channel 3 --------
#define PIN_CH3_GA (8)
#define PIN_CH3_GB (9)
#define CH3_DAC_CHANNEL (2)
#define CH3_ADC_CHANNEL (2)

// -------- Channel 4 --------
#define PIN_CH4_GA (6)
#define PIN_CH4_GB (7)
#define CH4_DAC_CHANNEL (3)
#define CH4_ADC_CHANNEL (3)

#endif // _SW22_H