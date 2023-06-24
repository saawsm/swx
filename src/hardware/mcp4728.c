#include "../swx.h"

#ifdef USE_DAC_MCP4728
#include "../util/i2c.h"
#include "../util/gpio.h"

#define MCP4728_MAX_VALUE (4095)
#define MCP4728_CHANNEL_COUNT (4)

#define MCP4728_CMD_WRITE_MULTI_IR (0x40)        // Sequential Multi-Write for DAC Input Registers
#define MCP4728_CMD_WRITE_MULTI_IR_EEPROM (0x50) // Sequential Write for DAC Input Registers and EEPROM

typedef enum { MCP4728_GAIN_ONE = 0, MCP4728_GAIN_TWO } mcp4728_gain_t;
typedef enum { MCP4728_VREF_VDD = 0, MCP4728_VREF_INTERNAL } mcp4728_vref_t;
typedef enum { MCP4728_PD_NORMAL = 0, MCP4728_PD_GND_1K, MCP4728_PD_GND_100K, MCP4728_PD_GND_500K } mcp4728_pd_mode_t;

#define MCP4728_GAIN (MCP4728_GAIN_ONE)
#define MCP4728_VREF (MCP4728_VREF_VDD)
#define MCP4728_PD (MCP4728_PD_NORMAL)
#define MCP4728_UDAC (false)

void init_dac() {
#ifdef PIN_LDAC
   init_gpio(PIN_LDAC, GPIO_OUT, 0); // active low
#endif
}

// Based on https://github.com/adafruit/Adafruit_MCP4728/blob/6d389cd87a8bd1e898136b4425c55ca7b83eccee/Adafruit_MCP4728.cpp
bool set_dac_direct(uint8_t channel, uint16_t value) {
   uint8_t buffer[3];

#ifdef I2C_CHECK_WRITE
   if (i2c_get_write_available(I2C_PORT_PERIF) < sizeof(buffer)) { // Check if we can write without blocking
      LOG_WARN("MCP4728 - I2C buffer full!\n");
      return false;
   }
#endif

   if (channel >= MCP4728_CHANNEL_COUNT) {
      LOG_WARN("MCP4728: ch=%u - Out of range channel index!\n", channel);
      return false;
   }

   if (value > MCP4728_MAX_VALUE)
      value = MCP4728_MAX_VALUE;

   // ------------------------------------------------------------------------------------------------
   // |             0                 |               1                 |             2              |
   // ------------------------------------------------------------------------------------------------
   // C2 C1 C0 W1 W2 DAC1 DAC0 ~UDAC [A] VREF PD1 PD0 Gx D11 D10 D9 D8 [A] D7 D6 D5 D4 D3 D2 D1 D0 [A]

   buffer[0] = MCP4728_CMD_WRITE_MULTI_IR | (channel << 1) | MCP4728_UDAC;

   value |= MCP4728_VREF << 15;
   value |= MCP4728_PD << 13;
   value |= MCP4728_GAIN << 12;

   buffer[1] = value >> 8;
   buffer[2] = value & 0xFF;

   LOG_FINE("set_dac_direct: ch=%u value=%u\n", channel, value);

   int ret = i2c_write(I2C_PORT_PERIF, DAC_ADDRESS, buffer, sizeof(buffer), false, I2C_DEVICE_TIMEOUT);
   if (ret < 0) {
      LOG_ERROR("MCP4728: ret=%d - I2C write failed!\n", ret);
      return false;
   }
   return true;
}
#endif