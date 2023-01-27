#include "pulse_func_gen.h"

constexpr swx::PulseFuncGenerator::Param STATE_SEQUENCE[4] = {
    swx::PulseFuncGenerator::OFF_TIME,
    swx::PulseFuncGenerator::ON_RAMP_TIME,
    swx::PulseFuncGenerator::ON_TIME,
    swx::PulseFuncGenerator::OFF_RAMP_TIME,
};

swx::PulseFuncGenerator::PulseFuncGenerator(Output& output) : output(output) {
   for (uint8_t channel = 0; channel < CHANNEL_COUNT; channel++) {
      ChannelInfo& ch = channels[channel];

      ch.nextPulseTime = 0;
      ch.nextStateTime = 0;
      ch.stateIndex = 0;
      ch.power = 0.5f;

      if (((CH_ENABLED >> channel) & 1) == 0) {  // Dont enable generator slots for disabled channels by default
         ch.source = NONE;
      } else {
         ch.source = BASIC;
      }

      setParameter(channel, FREQUENCY, MAX, 5000);

      setParameter(channel, PULSE_WIDTH, MAX, 255);

      setParameter(channel, ON_TIME, MAX, UINT16_MAX);
      setParameter(channel, ON_RAMP_TIME, MAX, UINT16_MAX);

      setParameter(channel, OFF_TIME, MAX, UINT16_MAX);
      setParameter(channel, OFF_RAMP_TIME, MAX, UINT16_MAX);
   }
}

void swx::PulseFuncGenerator::process() {
   // Update dynamic parameters
   for (uint8_t i = 0; i < TOTAL_PARAMETERS; i++) parameters[i].update();

   for (uint8_t channel = 0; channel < CHANNEL_COUNT; channel++) {
      ChannelInfo& ch = channels[channel];
      
      if (ch.source == NONE) continue;

      uint32_t time = time_us_32();
      if (time > ch.nextStateTime) {
         if (++ch.stateIndex >= 4) ch.stateIndex = 0;  // increment or reset after 4 states (off, on_ramp, on, off_ramp)

         // set the next state time based on the state parameter (off_time, on_ramp_time, on_time, off_ramp_time)
         ch.nextStateTime = time + (getParameter(channel, STATE_SEQUENCE[ch.stateIndex], VALUE) * 1000);
      }

      float powerModifier = 1.0f;

      // scale power level depending on the current state (e.g. transition between off and on)
      time = time_us_32();
      Param channelState = STATE_SEQUENCE[ch.stateIndex];
      switch (channelState) {
         case ON_RAMP_TIME:     // ramp power from zero to power value
         case OFF_RAMP_TIME: {  // ramp power from power value to zero
            uint16_t rampTime = getParameter(channel, channelState, VALUE) * 1000;
            if (rampTime == 0) break;

            uint16_t timeRemaining = ch.nextStateTime - time;

            powerModifier = (float)timeRemaining / rampTime;
            if (powerModifier > 1) powerModifier = 1;

            if (channelState == ON_RAMP_TIME) powerModifier = 1.0f - powerModifier;  // invert percentage if transition is going from off to on
            break;
         }

         case OFF_TIME: {  // if the state is off, continue to next channel without pulsing
            output.setPower(channel, 0);
            continue;
         }
         default:
            break;
      }

      powerModifier *= channels[channel].power;  // apply the general channel power modifier to the 'state' power modifier

      switch (ch.source) {
         case BASIC: {  // basic pulse gen
            uint16_t power = getParameter(channel, POWER, VALUE);
            if (power == 0) break;

            // Set channel output power
            output.setPower(channel, static_cast<uint16_t>(powerModifier * power));

            time = time_us_32();
            if (time > ch.nextPulseTime) {
               uint16_t frequency = getParameter(channel, FREQUENCY, VALUE);
               uint16_t pulseWidth = getParameter(channel, PULSE_WIDTH, VALUE);

               if (frequency == 0 || pulseWidth == 0) break;

               ch.nextPulseTime = time + (10000000 / frequency);  // dHz -> us

               // Pulse the channnel
               output.pulse(channel, pulseWidth);
            }
            break;
         }

         default:
            break;
      }
   }
}

//
// #################################################################################################################
//

swx::PulseFuncGenerator::Parameter::Parameter() : dir(1), nextUpdateTime(0) {
   for (uint8_t i = 0; i < TOTAL_TARGETS; i++) values[i] = 0;

   values[MAX] = 1000;
   values[MODE] = DISABLED;
}

void swx::PulseFuncGenerator::Parameter::update() {
   // skip update if parameter is static
   if (values[MODE] == DISABLED || values[RATE] == 0 || step == 0) return;

   if (time_us_32() < nextUpdateTime) return;

   nextUpdateTime = time_us_32() + updatePeriod;

   // TODO: Handle potential overflow/underflow when step is 2 or more

   values[VALUE] += step * dir;  // inc/dec parameter

   if (values[VALUE] <= values[MIN] || values[VALUE] >= values[MAX]) {
      switch (values[MODE]) {
         case UP_DOWN:
            dir *= -1;
            break;
         case DOWN_RESET:
            values[VALUE] = values[MAX];
            break;
         case UP_RESET:
            values[VALUE] = values[MIN];
            break;
         default:
            break;
      }
   }
}

constexpr uint16_t clamp(uint16_t value, uint16_t min, uint16_t max) {
   return (value < min ? min : (value > max ? max : value));
}

void swx::PulseFuncGenerator::Parameter::set(Target target, uint16_t value) {
   values[target] = value;

   // Determine steps and update period based on cycle rate
   if (values[MODE] != DISABLED && values[RATE] != 0) {
      step = 1;  // start at 1 step for highest resolution
      while (step < 100) {
         uint32_t delta = (values[MAX] - values[MIN]) / step;

         if (delta == 0) {  // if value range is zero, soft disable stepping
            step = 0;
            break;
         }

         // number of microseconds it takes to go from one extent to another
         // rate is currently in millihertz, making the max rate be ~65 Hz
         uint32_t period = 1000000000 / values[RATE];

         if (period >= delta) {             // delay is 1us or greater
            updatePeriod = period / delta;  // number of microseconds between each value
            break;
         } else {
            step++;
         }
      }
   }

   // ensure parameter values are valid
   switch (target) {
      case MIN:
      case MAX:
      case VALUE:
         values[VALUE] = clamp(values[VALUE], values[MIN], values[MAX]);
         break;
      case MODE:
         if (value == UP_RESET) {
            dir = 1;
         } else if (value == DOWN_RESET) {
            dir = -1;
         }
         break;
      default:
         break;
   }
}