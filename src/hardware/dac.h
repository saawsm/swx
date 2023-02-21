#ifndef _DAC_H
#define _DAC_H

#include "../swx.h"

namespace swx {

   struct Dac {
      const uint8_t channelCount; // the number of channels this dac has
      const uint16_t maxValue;    // the maximum set value for each channel

      Dac(uint8_t channelCount, uint16_t maxValue) : channelCount(channelCount), maxValue(maxValue) {}

      virtual ~Dac() = default;

      virtual bool setChannel(uint8_t channel, uint16_t value) = 0;
   };

} // namespace swx

#endif // _DAC_H