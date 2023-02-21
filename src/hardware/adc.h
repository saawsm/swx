#ifndef _ADC_H
#define _ADC_H

#include "../swx.h"

namespace swx {

   struct Adc {
      const uint8_t channelCount; // the number of channels this adc has

      Adc(uint8_t channelCount) : channelCount(channelCount) {}

      virtual ~Adc() = default;

      virtual bool startConversion(uint8_t channel, bool continuous = false) = 0;

      virtual bool conversionComplete() = 0;

      virtual int16_t lastConversion() = 0;

      virtual float computeVolts(int16_t counts) = 0;
   };

} // namespace swx

#endif // _ADC_H