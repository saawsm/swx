#include "../swx.h"

#ifdef USE_ADC_ADS1015
#include "../util/i2c.h"

#define ADS1015_CHANNEL_COUNT (4)

/* POINTER REGISTER */
#define ADS1015_REG_POINTER_MASK (0x03)      // Point mask
#define ADS1015_REG_POINTER_CONVERT (0x00)   // Conversion
#define ADS1015_REG_POINTER_CONFIG (0x01)    // Configuration
#define ADS1015_REG_POINTER_LOWTHRESH (0x02) // Low threshold
#define ADS1015_REG_POINTER_HITHRESH (0x03)  // High threshold

/* CONFIG REGISTER */
#define ADS1015_REG_CONFIG_OS_MASK (0x8000)    // OS Mask
#define ADS1015_REG_CONFIG_OS_SINGLE (0x8000)  // Write: Set to start a single-conversion
#define ADS1015_REG_CONFIG_OS_BUSY (0x0000)    // Read: Bit = 0 when conversion is in progress
#define ADS1015_REG_CONFIG_OS_NOTBUSY (0x8000) // Read: Bit = 1 when device is not performing a conversion

#define ADS1015_REG_CONFIG_MUX_MASK (0x7000)   // Mux Mask

uint16_t ADS1015_CHANNEL_TO_MUX[4] = {
    0x4000, // AIN0
    0x5000, // AIN1
    0x6000, // AIN2
    0x7000  // AIN3
};

#define ADS1015_REG_CONFIG_PGA_MASK (0x0E00)     // PGA Mask
#define ADS1015_REG_CONFIG_PGA_6_144V (0x0000)   // +/-6.144V range = Gain 2/3
#define ADS1015_REG_CONFIG_PGA_4_096V (0x0200)   // +/-4.096V range = Gain 1
#define ADS1015_REG_CONFIG_PGA_2_048V (0x0400)   // +/-2.048V range = Gain 2 (default)
#define ADS1015_REG_CONFIG_PGA_1_024V (0x0600)   // +/-1.024V range = Gain 4
#define ADS1015_REG_CONFIG_PGA_0_512V (0x0800)   // +/-0.512V range = Gain 8
#define ADS1015_REG_CONFIG_PGA_0_256V (0x0A00)   // +/-0.256V range = Gain 16

#define ADS1015_REG_CONFIG_MODE_MASK (0x0100)    // Mode Mask
#define ADS1015_REG_CONFIG_MODE_CONTIN (0x0000)  // Continuous conversion mode
#define ADS1015_REG_CONFIG_MODE_SINGLE (0x0100)  // Power-down single-shot mode (default)

#define ADS1015_REG_CONFIG_RATE_MASK (0x00E0)    // Data Rate Mask

#define ADS1015_REG_CONFIG_CMODE_MASK (0x0010)   // CMode Mask
#define ADS1015_REG_CONFIG_CMODE_TRAD (0x0000)   // Traditional comparator with hysteresis (default)
#define ADS1015_REG_CONFIG_CMODE_WINDOW (0x0010) // Window comparator

#define ADS1015_REG_CONFIG_CPOL_MASK (0x0008)    // CPol Mask
#define ADS1015_REG_CONFIG_CPOL_ACTVLOW (0x0000) // ALERT/RDY pin is low when active (default)
#define ADS1015_REG_CONFIG_CPOL_ACTVHI (0x0008)  // ALERT/RDY pin is high when active

#define ADS1015_REG_CONFIG_CLAT_MASK (0x0004)    // Determines if ALERT/RDY pin latches once asserted
#define ADS1015_REG_CONFIG_CLAT_NONLAT (0x0000)  // Non-latching comparator (default)
#define ADS1015_REG_CONFIG_CLAT_LATCH (0x0004)   // Latching comparator

#define ADS1015_REG_CONFIG_CQUE_MASK (0x0003)    // CQue Mask
#define ADS1015_REG_CONFIG_CQUE_1CONV (0x0000)   // Assert ALERT/RDY after one conversions
#define ADS1015_REG_CONFIG_CQUE_2CONV (0x0001)   // Assert ALERT/RDY after two conversions
#define ADS1015_REG_CONFIG_CQUE_4CONV (0x0002)   // Assert ALERT/RDY after four conversions
#define ADS1015_REG_CONFIG_CQUE_NONE (0x0003)    // Disable the comparator and put ALERT/RDY in high state (default)

typedef enum Rate {
   ADS1015_RATE_128SPS = 0x0000,  // 128 samples per second
   ADS1015_RATE_250SPS = 0x0020,  // 250 samples per second
   ADS1015_RATE_490SPS = 0x0040,  // 490 samples per second
   ADS1015_RATE_920SPS = 0x0060,  // 920 samples per second
   ADS1015_RATE_1600SPS = 0x0080, // 1600 samples per second (default)
   ADS1015_RATE_2400SPS = 0x00A0, // 2400 samples per second
   ADS1015_RATE_3300SPS = 0x00C0  // 3300 samples per second
} ads1015_rate_t;

typedef enum {
   ADS1015_GAIN_TWOTHIRDS = ADS1015_REG_CONFIG_PGA_6_144V,
   ADS1015_GAIN_ONE = ADS1015_REG_CONFIG_PGA_4_096V,
   ADS1015_GAIN_TWO = ADS1015_REG_CONFIG_PGA_2_048V,
   ADS1015_GAIN_FOUR = ADS1015_REG_CONFIG_PGA_1_024V,
   ADS1015_GAIN_EIGHT = ADS1015_REG_CONFIG_PGA_0_512V,
   ADS1015_GAIN_SIXTEEN = ADS1015_REG_CONFIG_PGA_0_256V
} ads1015_gain_t;

#define ADS1015_GAIN (ADS1015_GAIN_ONE)
#define ADS1015_RATE (ADS1015_RATE_3300SPS)

static uint8_t buffer[3];

static bool write_register(uint8_t reg, uint16_t value) {
   buffer[0] = reg;
   buffer[1] = value >> 8;
   buffer[2] = value & 0xFF;

   int ret = i2c_write(I2C_PORT_PERIF, ADC_ADDRESS, buffer, sizeof(buffer), false, I2C_DEVICE_TIMEOUT);
   if (ret < 0) {
      LOG_ERROR("ADS1015:write_register: ret=%d - I2C write failed!\n", ret);
      return false;
   }
   return true;
}

static uint16_t read_register(uint8_t reg) {
   buffer[0] = reg;

   int ret = i2c_write(I2C_PORT_PERIF, ADC_ADDRESS, buffer, 1, true, I2C_DEVICE_TIMEOUT);
   if (ret < 0) {
      LOG_ERROR("ADS1015:write_register: ret=%d - I2C write failed!\n", ret);
      return 0;
   }

   ret = i2c_read(I2C_PORT_PERIF, ADC_ADDRESS, buffer, 2, false, I2C_DEVICE_TIMEOUT);
   if (ret < 0) {
      LOG_ERROR("ADS1015:read_register: ret=%d - I2C read failed!\n", ret);
      return 0;
   }

   return (buffer[0] << 8) | buffer[1];
}

void init_adc() {}

bool adc_read_counts(uint8_t channel, uint16_t* counts) {
   if (channel >= ADS1015_CHANNEL_COUNT) {
      LOG_WARN("ADS1015: ch=%u - Out of range channel index!\n", channel);
      return false;
   }

   uint16_t config = ADS1015_REG_CONFIG_CQUE_NONE |    // Set CQUE to disabled
                     ADS1015_REG_CONFIG_CLAT_NONLAT |  // Non-latching (default val)
                     ADS1015_REG_CONFIG_CPOL_ACTVLOW | // Alert/Rdy active low   (default val)
                     ADS1015_REG_CONFIG_CMODE_TRAD;    // Traditional comparator (default val)

   // config |= ADS1015_REG_CONFIG_MODE_CONTIN;     // Continuous sampling
   config |= ADS1015_REG_CONFIG_MODE_SINGLE;                // Single shot mode

   config |= ADS1015_GAIN;                                  // Set PGA gain
   config |= ADS1015_RATE;                                  // Set sample rate
   config |= ADS1015_CHANNEL_TO_MUX[channel];               // Set mux for single ended / specified channel

   config |= ADS1015_REG_CONFIG_OS_SINGLE;                  // Set single-conversion bit

   if (!write_register(ADS1015_REG_POINTER_CONFIG, config)) // Start conversion
      return false;

   // Wait for conversion to complete
   while ((read_register(ADS1015_REG_POINTER_CONFIG) & ADS1015_REG_CONFIG_OS_MASK) == ADS1015_REG_CONFIG_OS_BUSY)
      tight_loop_contents();

   // Read conversion results
   uint16_t res = read_register(ADS1015_REG_POINTER_CONVERT) >> 4;

   // Shift 12-bit results right 4 bits, making sure we keep the sign bit intact
   //  if (res > 0x07FF) // negative number - extend the sign to 16th bit
   //   res |= 0xF000;

   // return (int16_t)res;
   *counts = res;
   return true;
}

float adc_compute_volts(uint16_t counts) {
   float fsRange;
   switch (ADS1015_GAIN) {
      case ADS1015_GAIN_TWOTHIRDS:
         fsRange = 6.144f;
         break;
      case ADS1015_GAIN_ONE:
         fsRange = 4.096f;
         break;
      case ADS1015_GAIN_TWO:
         fsRange = 2.048f;
         break;
      case ADS1015_GAIN_FOUR:
         fsRange = 1.024f;
         break;
      case ADS1015_GAIN_EIGHT:
         fsRange = 0.512f;
         break;
      case ADS1015_GAIN_SIXTEEN:
         fsRange = 0.256f;
         break;
      default:
         fsRange = 0.0f;
   }
   return counts * (fsRange / 2048);
}

#endif