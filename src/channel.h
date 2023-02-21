#ifndef _CHANNEL_H
#define _CHANNEL_H

#include "swx.h"

#include <hardware/pio.h>

#include "hardware/adc.h"
#include "hardware/dac.h"

#include "pulse_queue.h"

// #define CH_IGNORE_CAL_ERRORS  // Ignore calibration errors - Caution!

#ifdef CH_IGNORE_CAL_ERRORS
   #warning "Channel calibration errors are ignored! Output modules could overload!"
#endif

namespace swx {

   class Channel final {
   public:
      enum Status : uint8_t { INVALID = 0, FAULT, UNINITIALIZED, READY, CALIBRATING };

      const uint8_t index;

      const uint8_t pinGateA; // The gate pin for leg A
      const uint8_t pinGateB; // The gate pin for leg B

      const uint8_t dacChannel;
      const uint8_t adcChannel;

      const float calThresholdOk;
      const float calThresholdOver;
      const uint16_t calOffPoint;

   private:
      PulseQueue& queue;
      Adc& adc;
      Dac& dac;

      pio_hw_t* const pio;
      const uint8_t sm;
      const uint pio_program_offset;

      uint16_t calValue;

      Status status;

   public:
      Channel(uint8_t index, uint8_t pinGateA, uint8_t pinGateB, Adc& adc, uint8_t adcChannel, Dac& dac, uint8_t dacChannel, PulseQueue& queue, pio_hw_t* pio, uint8_t sm,
              uint pio_program_offset, float calThresholdOk, float calThresholdOver, uint16_t calOffPoint);
      ~Channel();

      Channel(const Channel&) = delete;
      Channel& operator=(const Channel&) = delete;
      Channel(Channel&&) = delete;
      Channel& operator=(Channel&&) = delete;

      void setPower(uint16_t power);

      void pulse(uint16_t pos_us, uint16_t neg_us);
      inline void pulse(uint16_t us) { pulse(us, us); }

      void immediatePulse(uint16_t pos_us, uint16_t neg_us);
      inline void immediatePulse(uint16_t us) { immediatePulse(us, us); }

      Status calibrate();

      Status getStatus() const { return status; }

      uint16_t getCalibration() const { return calValue; }

      void characterize();

   private:
      float readVoltage();
   };

} // namespace swx

#endif // _CHANNEL_H