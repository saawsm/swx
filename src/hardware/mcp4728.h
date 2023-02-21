#ifndef _MCP4728_H
#define _MCP4728_H

#include "dac.h"

#include <hardware/i2c.h>

#define MCP4728_BITS (12)
#define MCP4728_MAX_VALUE (4095)
#define MCP4728_CHANNEL_COUNT (4)

#define MCP4728_CMD_WRITE_MULTI_IR (0x40)        // Sequential Multi-Write for DAC Input Registers
#define MCP4728_CMD_WRITE_MULTI_IR_EEPROM (0x50) // Sequential Write for DAC Input Registers and EEPROM

namespace swx {

   class MCP4728 final : public Dac {
      i2c_inst_t* const i2c;
      const uint8_t address;

      uint8_t buffer[3];

   public:
      enum Gain { GAIN_ONE = 0, GAIN_TWO };
      enum VRef { VREF_VDD = 0, VREF_INTERNAL };
      enum PowerDownMode { PD_NORMAL = 0, PD_GND_1K, PD_GND_100K, PD_GND_500K };

      Gain gain;
      VRef vref;
      PowerDownMode pdMode;
      bool udac; // DAC latch, upload values to output register from DAC input register. If the LDAC pin is low this is unnecessary.

   public:
      MCP4728(i2c_inst_t* i2c, const uint8_t address);

      virtual bool setChannel(uint8_t channel, uint16_t value) override { return setChannel(channel, value, vref, gain, pdMode, udac); }

      bool setChannel(uint8_t channel, uint16_t value, VRef vref, Gain gain, PowerDownMode pdMode, bool udac);

      // bool save();
   };

} // namespace swx

#endif // _MCP4728_H