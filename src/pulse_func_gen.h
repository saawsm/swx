#ifndef _PULSE_FUNC_GEN_H
#define _PULSE_FUNC_GEN_H

#include "swx.h"

#include "output.h"

namespace swx {

   class PulseFuncGenerator final {
   public:
      enum PulseSrc : uint8_t {
         NONE = 0,
         BASIC,
      };

      enum Param : uint8_t {
         /// The intensity of the signal as a percent (0 to 1000).
         POWER = 0,

         /// The frequency of pulses generated in dHz (decihertz, 1 Hz = 10 dHz)
         FREQUENCY,

         /// The duration of each pulse (0 us to 255 us).
         PULSE_WIDTH,

         /// The number of milliseconds the output is on.
         ON_TIME,

         /// The duration in milliseconds to smoothly ramp intensity level when going from off to on. `_|‾`
         ON_RAMP_TIME,

         /// The number of milliseconds the output is off.
         OFF_TIME,

         /// The duration in milliseconds to smoothly ramp intensity level when going from on to off. `‾|_`
         OFF_RAMP_TIME,

         TOTAL_PARAMS // Number of parameters in enum, used for arrays
      };

      enum Target : uint8_t {
         /// The actual parameter value.
         VALUE = 0,

         /// The minimum the parameter value can be (used by rate for auto cycling value)
         MIN,

         /// The maximum the parameter value can be (used by rate for auto cycling value)
         MAX,

         /// The frequency of parameter cycling in mHz (millihertz, 1 Hz = 1000 mHz)
         RATE,

         /// The cycling mode the parameter is using. Determines how the value resets when it reaches min/max.
         MODE,

         TOTAL_TARGETS // Number of targets in enum, used for arrays
      };

      enum Mode : uint8_t {
         DISABLED = 0,

         /// Ramp smoothly between minimum and maximum.
         UP_DOWN,

         /// Ramp smoothly from minimum to maximum and then reset at minimum again.
         UP_RESET,

         /// Ramp smoothly from maximum to minimum and then reset at maximum again.
         DOWN_RESET,
      };

   private:
      class Parameter final {
         uint16_t values[TOTAL_TARGETS];

         int8_t step;
         uint32_t nextUpdateTime;
         uint32_t updatePeriod;

      public:
         Parameter();

         void update();

         void set(Target target, uint16_t value);

         uint16_t get(Target target) const { return values[target]; }
      };

      struct ChannelInfo final {
         PulseSrc source;
         float powerModifier;

         uint32_t nextPulseTime;
         uint32_t nextStateTime;
         uint8_t stateIndex;
      };

      Output& output;
      ChannelInfo channels[CHANNEL_COUNT];

      static constexpr uint8_t TOTAL_PARAMETERS = TOTAL_PARAMS * CHANNEL_COUNT;
      Parameter parameters[TOTAL_PARAMETERS];

   public:
      PulseFuncGenerator(Output& output);

      void process();

      void setSource(uint8_t channel, PulseSrc src) { channels[channel].source = src; }
      PulseSrc getSource(uint8_t channel) const { return channels[channel].source; }

      void setParameter(uint8_t channel, Param param, uint16_t value) { setParameter(channel, param, Target::VALUE, value); }
      void setParameter(uint8_t channel, Param param, Target target, uint16_t value) { return parameters[(channel * TOTAL_PARAMS) + param].set(target, value); }

      uint16_t getParameter(uint8_t channel, Param param, Target target = Target::VALUE) const {
         return parameters[(channel * TOTAL_PARAMS) + param].get(target);
      }

      void setPower(uint8_t channel, uint16_t value) {
         if (value > 1000)
            value = 1000;
         channels[channel].powerModifier = (float)value / 1000.0f;
      }
      uint16_t getPower(uint8_t channel) const { return channels[channel].powerModifier; }
   };

} // namespace swx

#endif // _PULSE_FUNC_GEN_H