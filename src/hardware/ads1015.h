#ifndef _ADS1015_H
#define _ADS1015_H

#include "adc.h"

#include <hardware/i2c.h>

#define ADS1015_CHANNEL_COUNT (4)

/* POINTER REGISTER */
#define ADS1015_REG_POINTER_MASK (0x03)       // Point mask
#define ADS1015_REG_POINTER_CONVERT (0x00)    // Conversion
#define ADS1015_REG_POINTER_CONFIG (0x01)     // Configuration
#define ADS1015_REG_POINTER_LOWTHRESH (0x02)  // Low threshold
#define ADS1015_REG_POINTER_HITHRESH (0x03)   // High threshold

/* CONFIG REGISTER */
#define ADS1015_REG_CONFIG_OS_MASK (0x8000)     // OS Mask
#define ADS1015_REG_CONFIG_OS_SINGLE (0x8000)   // Write: Set to start a single-conversion
#define ADS1015_REG_CONFIG_OS_BUSY (0x0000)     // Read: Bit = 0 when conversion is in progress
#define ADS1015_REG_CONFIG_OS_NOTBUSY (0x8000)  // Read: Bit = 1 when device is not performing a conversion

#define ADS1015_REG_CONFIG_MUX_MASK (0x7000)  // Mux Mask

constexpr uint16_t ADS1015_CHANNEL_TO_MUX[] = {
    0x4000,  // AIN0
    0x5000,  // AIN1
    0x6000,  // AIN2
    0x7000   // AIN3
};

#define ADS1015_REG_CONFIG_PGA_MASK (0x0E00)    // PGA Mask
#define ADS1015_REG_CONFIG_PGA_6_144V (0x0000)  // +/-6.144V range = Gain 2/3
#define ADS1015_REG_CONFIG_PGA_4_096V (0x0200)  // +/-4.096V range = Gain 1
#define ADS1015_REG_CONFIG_PGA_2_048V (0x0400)  // +/-2.048V range = Gain 2 (default)
#define ADS1015_REG_CONFIG_PGA_1_024V (0x0600)  // +/-1.024V range = Gain 4
#define ADS1015_REG_CONFIG_PGA_0_512V (0x0800)  // +/-0.512V range = Gain 8
#define ADS1015_REG_CONFIG_PGA_0_256V (0x0A00)  // +/-0.256V range = Gain 16

#define ADS1015_REG_CONFIG_MODE_MASK (0x0100)    // Mode Mask
#define ADS1015_REG_CONFIG_MODE_CONTIN (0x0000)  // Continuous conversion mode
#define ADS1015_REG_CONFIG_MODE_SINGLE (0x0100)  // Power-down single-shot mode (default)

#define ADS1015_REG_CONFIG_RATE_MASK (0x00E0)  // Data Rate Mask

#define ADS1015_REG_CONFIG_CMODE_MASK (0x0010)    // CMode Mask
#define ADS1015_REG_CONFIG_CMODE_TRAD (0x0000)    // Traditional comparator with hysteresis (default)
#define ADS1015_REG_CONFIG_CMODE_WINDOW (0x0010)  // Window comparator

#define ADS1015_REG_CONFIG_CPOL_MASK (0x0008)     // CPol Mask
#define ADS1015_REG_CONFIG_CPOL_ACTVLOW (0x0000)  // ALERT/RDY pin is low when active (default)
#define ADS1015_REG_CONFIG_CPOL_ACTVHI (0x0008)   // ALERT/RDY pin is high when active

#define ADS1015_REG_CONFIG_CLAT_MASK (0x0004)    // Determines if ALERT/RDY pin latches once asserted
#define ADS1015_REG_CONFIG_CLAT_NONLAT (0x0000)  // Non-latching comparator (default)
#define ADS1015_REG_CONFIG_CLAT_LATCH (0x0004)   // Latching comparator

#define ADS1015_REG_CONFIG_CQUE_MASK (0x0003)   // CQue Mask
#define ADS1015_REG_CONFIG_CQUE_1CONV (0x0000)  // Assert ALERT/RDY after one conversions
#define ADS1015_REG_CONFIG_CQUE_2CONV (0x0001)  // Assert ALERT/RDY after two conversions
#define ADS1015_REG_CONFIG_CQUE_4CONV (0x0002)  // Assert ALERT/RDY after four conversions
#define ADS1015_REG_CONFIG_CQUE_NONE (0x0003)   // Disable the comparator and put ALERT/RDY in high state (default)

namespace swx {

class ADS1015 final : public Adc {
   i2c_inst_t* const i2c;
   const uint8_t address;

   uint8_t buffer[3];

  public:
   enum Rate {
      RATE_128SPS = 0x0000,   // 128 samples per second
      RATE_250SPS = 0x0020,   // 250 samples per second
      RATE_490SPS = 0x0040,   // 490 samples per second
      RATE_920SPS = 0x0060,   // 920 samples per second
      RATE_1600SPS = 0x0080,  // 1600 samples per second (default)
      RATE_2400SPS = 0x00A0,  // 2400 samples per second
      RATE_3300SPS = 0x00C0   // 3300 samples per second
   };

   enum Gain {
      GAIN_TWOTHIRDS = ADS1015_REG_CONFIG_PGA_6_144V,
      GAIN_ONE = ADS1015_REG_CONFIG_PGA_4_096V,
      GAIN_TWO = ADS1015_REG_CONFIG_PGA_2_048V,
      GAIN_FOUR = ADS1015_REG_CONFIG_PGA_1_024V,
      GAIN_EIGHT = ADS1015_REG_CONFIG_PGA_0_512V,
      GAIN_SIXTEEN = ADS1015_REG_CONFIG_PGA_0_256V
   };

   Gain gain;
   Rate rate;

  public:
   ADS1015(i2c_inst_t* i2c, uint8_t address, Gain gain = GAIN_TWOTHIRDS, Rate rate = RATE_1600SPS);

   virtual bool startConversion(uint8_t channel, bool continuous) override;
   virtual bool conversionComplete() override;
   virtual int16_t lastConversion() override;

   virtual float computeVolts(int16_t counts) override;

  private:
   bool writeRegister(uint8_t reg, uint16_t value);
   uint16_t readRegister(uint8_t reg);
};

}  // namespace swx

#endif  // _ADS1015_H