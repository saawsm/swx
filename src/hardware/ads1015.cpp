#include "ads1015.h"

#include "../util.h"

swx::ADS1015::ADS1015(i2c_inst_t* i2c, uint8_t address, Gain gain, Rate rate)
    : Adc(ADS1015_CHANNEL_COUNT), i2c(i2c), address(address), gain(gain), rate(rate) {}

// Based on https://github.com/adafruit/Adafruit_ADS1X15/blob/2f58faf6e28108f46223940dfc7d8540303765bc/Adafruit_ADS1X15.cpp
bool swx::ADS1015::startConversion(uint8_t channel, bool continuous) {
   uint16_t config = ADS1015_REG_CONFIG_CQUE_NONE |     // Set CQUE to disabled
                     ADS1015_REG_CONFIG_CLAT_NONLAT |   // Non-latching (default val)
                     ADS1015_REG_CONFIG_CPOL_ACTVLOW |  // Alert/Rdy active low   (default val)
                     ADS1015_REG_CONFIG_CMODE_TRAD;     // Traditional comparator (default val)
   if (continuous) {
      config |= ADS1015_REG_CONFIG_MODE_CONTIN;  // Continuous sampling
   } else {
      config |= ADS1015_REG_CONFIG_MODE_SINGLE;  // Single shot mode
   }

   config |= gain;                             // Set PGA gain
   config |= rate;                             // Set sample rate
   config |= ADS1015_CHANNEL_TO_MUX[channel];  // Set mux for single ended / specified channel

   config |= ADS1015_REG_CONFIG_OS_SINGLE;  // Set single-conversion bit

   return writeRegister(ADS1015_REG_POINTER_CONFIG, config);  // Start conversion
}

bool swx::ADS1015::conversionComplete() {
   return (readRegister(ADS1015_REG_POINTER_CONFIG) & ADS1015_REG_CONFIG_OS_MASK) != ADS1015_REG_CONFIG_OS_BUSY;
}

int16_t swx::ADS1015::lastConversion() {
   uint16_t res = readRegister(ADS1015_REG_POINTER_CONVERT) >> 4;

   // Shift 12-bit results right 4 bits, making sure we keep the sign bit intact
   if (res > 0x07FF)  // negative number - extend the sign to 16th bit
      res |= 0xF000;

   return (int16_t)res;
}

float swx::ADS1015::computeVolts(int16_t counts) {  // see data sheet Table 3
   float fsRange;
   switch (gain) {
      case GAIN_TWOTHIRDS:
         fsRange = 6.144f;
         break;
      case GAIN_ONE:
         fsRange = 4.096f;
         break;
      case GAIN_TWO:
         fsRange = 2.048f;
         break;
      case GAIN_FOUR:
         fsRange = 1.024f;
         break;
      case GAIN_EIGHT:
         fsRange = 0.512f;
         break;
      case GAIN_SIXTEEN:
         fsRange = 0.256f;
         break;
      default:
         fsRange = 0.0f;
   }
   return counts * (fsRange / 2048);
}

bool swx::ADS1015::writeRegister(uint8_t reg, uint16_t value) {
   buffer[0] = reg;
   buffer[1] = value >> 8;
   buffer[2] = value & 0xFF;

   int ret = i2c_write(i2c, address, buffer, sizeof(buffer), false, I2C_DEVICE_TIMEOUT);
   if (ret < 0) {
      printf("[ADS1015::writeRegister] I2C write failed! ret=%d\n", ret);
      return false;
   }
   return true;
}

uint16_t swx::ADS1015::readRegister(uint8_t reg) {
   buffer[0] = reg;

   int ret = i2c_write(i2c, address, buffer, 1, true, I2C_DEVICE_TIMEOUT);
   if (ret < 0) {
      printf("[ADS1015::readRegister] I2C write failed! ret=%d\n", ret);
   }

   ret = i2c_read(i2c, address, buffer, 2, false, I2C_DEVICE_TIMEOUT);
   if (ret < 0) {
      printf("[ADS1015::readRegister] I2C read failed! ret=%d\n", ret);
   }

   return (buffer[0] << 8) | buffer[1];
}