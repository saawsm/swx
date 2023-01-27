#include "mcp4728.h"

#include "../util.h"

swx::MCP4728::MCP4728(i2c_inst_t* i2c, const uint8_t address)
    : Dac(MCP4728_CHANNEL_COUNT, MCP4728_MAX_VALUE), i2c(i2c), address(address), gain(GAIN_ONE), vref(VREF_VDD), pdMode(PD_NORMAL), udac(false) {}

// Based on https://github.com/adafruit/Adafruit_MCP4728/blob/6d389cd87a8bd1e898136b4425c55ca7b83eccee/Adafruit_MCP4728.cpp
bool swx::MCP4728::setChannel(uint8_t channel, uint16_t value, VRef vref, Gain gain, PowerDownMode pdMode, bool udac) {
#ifdef I2C_CHECK_WRITE
   if (i2c_get_write_available(i2c) < sizeof(buffer)) {  // check if we can write without blocking.
      printf("[MCP4728::setChannel] I2C buffer full!\n");
      return false;
   }
#endif

   // ------------------------------------------------------------------------------------------------
   // |             0                 |               1                 |             2              |
   // ------------------------------------------------------------------------------------------------
   // C2 C1 C0 W1 W2 DAC1 DAC0 ~UDAC [A] VREF PD1 PD0 Gx D11 D10 D9 D8 [A] D7 D6 D5 D4 D3 D2 D1 D0 [A]

   buffer[0] = MCP4728_CMD_WRITE_MULTI_IR | (channel << 1) | udac;

   value |= vref << 15;
   value |= pdMode << 13;
   value |= gain << 12;

   buffer[1] = value >> 8;
   buffer[2] = value & 0xFF;

   // printf("[MCP4728::setChannel] channel=%d value=%d\n", channel, value);

   int ret = i2c_write(i2c, address, buffer, sizeof(buffer), false, I2C_DEVICE_TIMEOUT);
   if (ret < 0) {
      printf("[MCP4728::setChannel] I2C write failed! ret=%d\n", ret);
      return false;
   }

   return true;
}

// bool swx::MCP4728::save() {
//    return false;  // TODO: add internal DAC EEPROM save - added safety for output modules, being POR low/off power level
// }