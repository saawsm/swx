#include "swx.h"
#include "parameter.h"
#include "output.h"

#define STATE_COUNT (4)

static const param_t STATE_SEQUENCE[STATE_COUNT] = {
    PARAM_OFF_TIME,
    PARAM_ON_RAMP_TIME,
    PARAM_ON_TIME,
    PARAM_OFF_RAMP_TIME,
};

static inline void parameter_update(parameter_t* p);

static inline uint16_t get_parameter(channel_t* ch, param_t param, target_t target) {
   return ch->parameters[param].values[target];
}

void pulse_gen_process() {
   extern channel_t channels[CHANNEL_COUNT];

   for (uint8_t channel = 0; channel < CHANNEL_COUNT; channel++) {
      channel_t* ch = &channels[channel];

      // Update dynamic parameters
      for (uint8_t i = 0; i < TOTAL_PARAMS; i++)
         parameter_update(&ch->parameters[i]);

      if (!ch->gen_enabled && ch->state_index == 0) // when disabled, wait until off state
         continue;

      uint32_t time = time_us_32();
      if (time > ch->next_state_time_us) {
         if (++ch->state_index >= STATE_COUNT)
            ch->state_index = 0; // Increment or reset after 4 states (off, on_ramp, on, off_ramp)

         // Set the next state time based on the state parameter (off_time, on_ramp_time, on_time, off_ramp_time)
         ch->next_state_time_us = time + (get_parameter(ch, STATE_SEQUENCE[ch->state_index], TARGET_VALUE) * 1000);
      }

      // TODO: Replace with fixed point ?
      float power_modifier = 1.0f;
      // Scale power level depending on the current state (e.g. transition between off and on)
      time = time_us_32();
      param_t channel_state = STATE_SEQUENCE[ch->state_index];
      switch (channel_state) {
         case PARAM_ON_RAMP_TIME:    // Ramp power from zero to power value
         case PARAM_OFF_RAMP_TIME: { // Ramp power from power value to zero
            uint16_t ramp_time = get_parameter(ch, channel_state, TARGET_VALUE) * 1000;
            if (ramp_time == 0)
               break;

            uint32_t time_remaining = ch->next_state_time_us - time;

            power_modifier = (float)time_remaining / ramp_time;
            if (power_modifier > 1)
               power_modifier = 1;

            if (channel_state == PARAM_ON_RAMP_TIME)
               power_modifier = 1.0f - power_modifier; // Invert percentage if transition is going from off to on
            break;
         }

         case PARAM_OFF_TIME: // If the state is off, continue to next channel without pulsing
            continue;
         default:
            break;
      }

      power_modifier *= ((float)ch->power_level / 1000.0f); // Combine the channel power modifier with the 'state' power modifier

      uint16_t power = get_parameter(ch, PARAM_POWER, TARGET_VALUE);
      if (power == 0)
         continue;

      // Set channel output power
      output_set_power(channel, (uint16_t)(power_modifier * power));

      // Generate the pulses
      time = time_us_32();
      if (time > ch->next_pulse_time_us) {
         uint16_t frequency = get_parameter(ch, PARAM_FREQUENCY, TARGET_VALUE);
         uint16_t pulse_width = get_parameter(ch, PARAM_PULSE_WIDTH, TARGET_VALUE);

         if (frequency == 0 || pulse_width == 0)
            continue;

         ch->next_pulse_time_us = time + (10000000 / frequency); // dHz -> us

         // Pulse the channnel
         output_pulse(channel, pulse_width, pulse_width, time_us_32());
      }
   }
}

static inline void parameter_update(parameter_t* p) {
   // Skip update if parameter is static
   if (p->values[TARGET_MODE] == TARGET_MODE_DISABLED || p->values[TARGET_RATE] == 0 || p->step == 0)
      return;

   if (time_us_32() < p->next_update_time_us)
      return;
   p->next_update_time_us = time_us_32() + p->update_period_us;

   const uint16_t previous_value = p->values[TARGET_VALUE]; // Save current value to check for wrapping

   p->values[TARGET_VALUE] += p->step;                      // Step value

   // If value reached min/max or wrapped
   if ((p->values[TARGET_VALUE] <= p->values[TARGET_MIN]) || (p->values[TARGET_VALUE] >= p->values[TARGET_MAX]) ||
       (p->step < 0 && p->values[TARGET_VALUE] > previous_value) || (p->step > 0 && p->values[TARGET_VALUE] < previous_value)) {
      switch (p->values[TARGET_MODE]) {
         case TARGET_MODE_DOWN_UP:
         case TARGET_MODE_UP_DOWN: { // Invert step direction if UP/DOWN mode
            p->values[TARGET_VALUE] = p->step < 0 ? p->values[TARGET_MIN] : p->values[TARGET_MAX];
            p->step *= -1;
            break;
         }
         case TARGET_MODE_UP_RESET: // Reset to MIN if sawtooth mode
            p->values[TARGET_VALUE] = p->values[TARGET_MIN];
            break;
         case TARGET_MODE_DOWN_RESET: // Reset to MAX if reversed sawtooth mode
            p->values[TARGET_VALUE] = p->values[TARGET_MAX];
            break;
         case TARGET_MODE_UP:
         case TARGET_MODE_DOWN: // Disable cycling for no-reset modes
            p->values[TARGET_MODE] = TARGET_MODE_DISABLED;
            break;
         default:
            break;
      }
   }
}

void parameter_set(parameter_t* p, target_t target, uint16_t value) {
   p->values[target] = value;

   // Determine steps and update period based on cycle rate
   if (p->values[TARGET_MODE] != TARGET_MODE_DISABLED && p->values[TARGET_RATE] != 0) {
      const int8_t previous_step = p->step;

      p->step = 1; // Start at 1 step for highest resolution
      while (p->step < 100) {
         uint32_t delta = (p->values[TARGET_MAX] - p->values[TARGET_MIN]) / p->step;

         if (delta == 0) { // If value range is zero, soft disable stepping
            p->step = 0;
            break;
         }

         // Number of microseconds it takes to go from one extent to another
         // rate is currently in millihertz, making the max rate be ~65 Hz
         uint32_t period = 1000000000 / p->values[TARGET_RATE];

         if (period >= delta) {                   // Delay is 1us or greater
            p->update_period_us = period / delta; // Number of microseconds between each value
            break;
         } else {
            p->step++; // Increase step so update period will be 1us or more
         }
      }

      // Invert step direction if MODE_DOWN or out of sync
      if (p->values[TARGET_MODE] == TARGET_MODE_DOWN_RESET || p->values[TARGET_MODE] == TARGET_MODE_DOWN ||
          (p->values[TARGET_MODE] == TARGET_MODE_DOWN_UP && previous_step > 0) || (p->values[TARGET_MODE] == TARGET_MODE_UP_DOWN && previous_step < 0))
         p->step = -(p->step);
   }
}