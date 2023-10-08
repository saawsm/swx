/*
 * swx
 * Copyright (C) 2023 saawsm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "pulse_gen.h"
#include "output.h"
#include "parameter.h"
#include "state.h"

#include "util/gpio.h"

#define STATE_COUNT (4)

static const param_t STATE_SEQUENCE[STATE_COUNT] = {
    PARAM_ON_RAMP_TIME,
    PARAM_ON_TIME,
    PARAM_OFF_RAMP_TIME,
    PARAM_OFF_TIME,
};

static channel_data_t channels[CHANNEL_COUNT];

static uint32_t sequencer_time_us;

extern void audio_process(channel_data_t* ch, uint8_t ch_index, uint16_t power);

static inline void parameter_step(uint8_t ch_index, param_t param);

void pulse_gen_init() {
   LOG_DEBUG("Init pulse generator...\n");

   sequencer_time_us = 0;

   // Set default parameter values
   for (uint8_t ch_index = 0; ch_index < CHANNEL_COUNT; ch_index++) {
      channels[ch_index].state_index = 0;

      SET_VALUE(ch_index, PARAM_POWER, TARGET_MAX, 1000);   // 100%
      SET_VALUE(ch_index, PARAM_POWER, TARGET_VALUE, 1000); // 100%

      SET_VALUE(ch_index, PARAM_FREQUENCY, TARGET_MAX, 5000);   // max. 500 Hz (soft limit - only auto cycling)
      SET_VALUE(ch_index, PARAM_FREQUENCY, TARGET_VALUE, 1800); // 180 Hz

      SET_VALUE(ch_index, PARAM_PULSE_WIDTH, TARGET_MAX, 500);   // max. 500 us
      SET_VALUE(ch_index, PARAM_PULSE_WIDTH, TARGET_VALUE, 150); // 150 us

      SET_VALUE(ch_index, PARAM_ON_TIME, TARGET_MAX, 10000);      // 10 seconds
      SET_VALUE(ch_index, PARAM_ON_RAMP_TIME, TARGET_MAX, 5000);  // 5 seconds
      SET_VALUE(ch_index, PARAM_OFF_TIME, TARGET_MAX, 10000);     // 10 seconds
      SET_VALUE(ch_index, PARAM_OFF_RAMP_TIME, TARGET_MAX, 5000); // 5 seconds
   }
}

void pulse_gen_process() {
   // update sequencer and active sequence mask
   const uint16_t seq_period_ms = get_state16(REG_SEQ_PERIOD);
   const uint8_t seq_count = get_state(REG_SEQ_COUNT);

   uint8_t sequencer_mask;
   if (seq_period_ms == 0 || seq_count == 0) {
      sequencer_mask = 0xff; // if sequencer is disabled, make mask all enabled
   } else {

      uint8_t sequencer_index = get_state(REG_SEQ_INDEX);
      if (sequencer_index >= MAX_SEQ_COUNT)
         sequencer_index = MAX_SEQ_COUNT - 1;

      uint32_t time = time_us_32();
      if (time > sequencer_time_us) {
         if (++sequencer_index >= seq_count || sequencer_index >= MAX_SEQ_COUNT)
            sequencer_index = 0; // Increment or reset after REG_SEQ_COUNT

         set_state(REG_SEQ_INDEX, sequencer_index);

         // Set the next sequencer time based on the REG_SEQ_PERIOD
         sequencer_time_us = time + (seq_period_ms * 1000);
      }

      sequencer_mask = get_state(REG_SEQn + sequencer_index);
   }

   // currently enabled channel mask based on REG_CH_GEN_ENABLE and current sequencer slot item
   const uint8_t en = get_state(REG_CH_GEN_ENABLE) & sequencer_mask;

   for (uint8_t ch_index = 0; ch_index < CHANNEL_COUNT; ch_index++) {
      channel_data_t* ch = &channels[ch_index];

      if (!(en & (1 << ch_index))) { // when disabled, hold state at zero
         ch->state_index = 0;
         ch->next_state_time_us = time_us_32() + (GET_VALUE(ch_index, STATE_SEQUENCE[ch->state_index], TARGET_VALUE) * 1000);
         continue;
      }

      // Update dynamic parameters
      for (uint8_t i = 0; i < TOTAL_PARAMS; i++)
         parameter_step(ch_index, i);

      uint32_t time = time_us_32();
      if (time > ch->next_state_time_us) {
         if (++ch->state_index >= STATE_COUNT)
            ch->state_index = 0; // Increment or reset after 4 states (on_ramp, on, off_ramp, off)

         // Set the next state time based on the state parameter (on_ramp_time, on_time, off_ramp_time, off_time)
         ch->next_state_time_us = time + (GET_VALUE(ch_index, STATE_SEQUENCE[ch->state_index], TARGET_VALUE) * 1000);
      }

      // TODO: Replace with fixed point ?
      float power_modifier = 1.0f;
      // Scale power level depending on the current state (e.g. transition between off and on)
      time = time_us_32();
      param_t channel_state = STATE_SEQUENCE[ch->state_index];
      switch (channel_state) {
         case PARAM_ON_RAMP_TIME:    // Ramp power from zero to power value
         case PARAM_OFF_RAMP_TIME: { // Ramp power from power value to zero
            uint16_t ramp_time = GET_VALUE(ch_index, channel_state, TARGET_VALUE) * 1000;
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

      // Combine the channel power modifier with the 'state' power modifier
      const uint16_t power_level = get_state16(REG_CHnn_POWER + (ch_index * 2));
      power_modifier *= ((float)(power_level < CHANNEL_POWER_MAX ? power_level : CHANNEL_POWER_MAX) / CHANNEL_POWER_MAX);

      uint16_t power = GET_VALUE(ch_index, PARAM_POWER, TARGET_VALUE);
      if (power == 0)
         continue;
      if (power > CHANNEL_POWER_MAX)
         power = CHANNEL_POWER_MAX;

      power = (uint16_t)(power_modifier * (float)power);

      // Channel has audio source, so process audio instead of processing function gen
      if (get_state(REG_CHn_AUDIO_SRC + ch_index) != 0) {
         audio_process(ch, ch_index, power);
      } else { // otherwise use pulse gen

         // Set channel output power, limit updates to ~2.2 kHz since it takes the DAC about ~110us/ch
         time = time_us_32();
         if (time - ch->last_power_time_us > 110 * CHANNEL_COUNT) {
            ch->last_power_time_us = time;
            output_set_power(ch_index, power);
         }

         // Generate the pulses
         time = time_us_32();
         if (time > ch->next_pulse_time_us) {
            const uint16_t frequency = GET_VALUE(ch_index, PARAM_FREQUENCY, TARGET_VALUE);
            const uint16_t pulse_width = GET_VALUE(ch_index, PARAM_PULSE_WIDTH, TARGET_VALUE);

            if (frequency == 0 || pulse_width == 0)
               continue;

            ch->next_pulse_time_us = time + (10000000ul / frequency); // dHz -> us

            // Pulse the channel
            output_pulse(ch_index, pulse_width, pulse_width, time_us_32());
         }
      }
   }
}

static inline void execute_action_list(uint8_t al_start, uint8_t al_end);

static inline void execute_action(uint8_t a_index) {
   if (a_index >= MAX_ACTIONS)
      return;

   const action_type_t type = get_state(REG_An_TYPE + a_index);
   if (type == ACTION_NONE)
      return;

   const uint8_t channel_mask = get_state(REG_An_CHANNEL_MASK + a_index);
   const uint16_t value = get_state16(REG_Ann_VALUE + (a_index * 2));

   switch (type) {
      case ACTION_SET:
      case ACTION_INCREMENT:
      case ACTION_DECREMENT: { // set,increment,decrement param+target value for all channels in mask, while keeping it constrained to TARGET_MIN/MAX
         const uint8_t param_target = get_state(REG_An_PARAM_TARGET + a_index);
         const param_t param = param_target >> 8;
         const target_t target = param_target & 0xff;

         if (param >= TOTAL_PARAMS || target >= TOTAL_TARGETS)
            return;

         uint16_t val = value;
         for (uint8_t ch_index = 0; ch_index < CHANNEL_COUNT; ch_index++) {
            if (~channel_mask & (1 << ch_index))
               continue;

            if (type == ACTION_INCREMENT) {
               val = GET_VALUE(ch_index, param, target) + value;
            } else if (type == ACTION_DECREMENT) {
               val = GET_VALUE(ch_index, param, target) - value;
            }

            if (target == TARGET_VALUE) {
               const uint16_t min = GET_VALUE(ch_index, param, TARGET_MIN);
               const uint16_t max = GET_VALUE(ch_index, param, TARGET_MAX);

               if (val > max) {
                  val = max;
               } else if (val < min) {
                  val = min;
               }
            }

            SET_VALUE(ch_index, param, target, val);
         }
         break;
      }
      case ACTION_ENABLE:
         set_state(REG_CH_GEN_ENABLE,
                   get_state(REG_CH_GEN_ENABLE) | channel_mask); // TODO: Support auto-disable channel generation using action value as delay in milliseconds
         break;
      case ACTION_DISABLE:
         set_state(REG_CH_GEN_ENABLE,
                   get_state(REG_CH_GEN_ENABLE) & ~channel_mask); // TODO: Support auto-enable channel generation using action value as delay in milliseconds
         break;
      case ACTION_TOGGLE:
         set_state(REG_CH_GEN_ENABLE,
                   get_state(REG_CH_GEN_ENABLE) ^ channel_mask); // TODO: Support auto-enable/disable channel generation using action value as delay in milliseconds
         break;
      case ACTION_EXECUTE:                              // run another action list from this list
         execute_action_list(value >> 8, value & 0xff); // start:upper byte, end: lower byte
         break;
      case ACTION_PARAM_UPDATE: { // update parameter step/rate using channel mask
         const uint8_t param_target = get_state(REG_An_PARAM_TARGET + a_index);
         const param_t param = param_target >> 8;
         for (uint8_t ch_index = 0; ch_index < CHANNEL_COUNT; ch_index++) {
            if (channel_mask & (1 << ch_index))
               parameter_update(ch_index, param);
         }
         break;
      }
      default:
         break;
   }
}

static inline void execute_action_list(uint8_t al_start, uint8_t al_end) {
   for (uint8_t i = al_start; i < al_end; i++)
      execute_action(i);
}

static inline void parameter_step(uint8_t ch_index, param_t param) {
   parameter_data_t* p = &channels[ch_index].parameters[param];

   // Get the mode without the notify bit
   const uint16_t mode_raw = GET_VALUE(ch_index, param, TARGET_MODE);
   const uint16_t mode = mode_raw & ~TARGET_MODE_NOTIFY_BIT;

   // Skip update if parameter is static
   if (mode == TARGET_MODE_DISABLED || GET_VALUE(ch_index, param, TARGET_RATE) == 0 || p->step == 0)
      return;

   if (time_us_32() < p->next_update_time_us) // only update at required time
      return;

   p->next_update_time_us = time_us_32() + p->update_period_us;

   const uint16_t previous_value = GET_VALUE(ch_index, param, TARGET_VALUE); // Save current value to check for wrapping
   uint16_t value = previous_value + p->step;                                // Step value

   const uint16_t min = GET_VALUE(ch_index, param, TARGET_MIN);
   const uint16_t max = GET_VALUE(ch_index, param, TARGET_MAX);

   // If value reached min/max or wrapped
   const bool end_reached = (value <= min) || (value >= max) || (p->step < 0 && value > previous_value) || (p->step > 0 && value < previous_value);
   if (end_reached) {
      switch (mode) {
         case TARGET_MODE_DOWN_UP:
         case TARGET_MODE_UP_DOWN: { // Invert step direction if UP/DOWN mode
            value = p->step < 0 ? min : max;
            p->step *= -1;
            break;
         }
         case TARGET_MODE_UP_RESET: // Reset to MIN if sawtooth mode
            value = min;
            break;
         case TARGET_MODE_DOWN_RESET: // Reset to MAX if reversed sawtooth mode
            value = max;
            break;
         case TARGET_MODE_UP: // Disable cycling for no-reset modes and clear the notify bit
            value = max;
            SET_VALUE(ch_index, param, TARGET_MODE, TARGET_MODE_DISABLED);
            break;
         case TARGET_MODE_DOWN: // Disable cycling for no-reset modes and clear the notify bit
            value = min;
            SET_VALUE(ch_index, param, TARGET_MODE, TARGET_MODE_DISABLED);
            break;
         default:
            return;
      }
   }

   SET_VALUE(ch_index, param, TARGET_VALUE, value); // Update value

   if (end_reached) {
      // parameter value extent reached, run action list if specified
      const uint16_t al = GET_VALUE(ch_index, param, TARGET_ACTION_RANGE);
      execute_action_list(al >> 8, al & 0xff); // start:upper byte, end: lower byte

      // if the notify bit is set, update flags and assert notify pin
      if (mode_raw & TARGET_MODE_NOTIFY_BIT) {
         const uint16_t address = REG_CHnn_PARAM_FLAGS + (ch_index * 2);
         set_state16(address, get_state16(address) | (1 << param));
         gpio_assert(PIN_INT);
      }
   }
}

void parameter_update(uint8_t ch_index, param_t param) {
   if (ch_index >= CHANNEL_COUNT || param >= TOTAL_PARAMS)
      return;

   // Get the mode without the notify bit
   const uint16_t mode = GET_VALUE(ch_index, param, TARGET_MODE) & ~TARGET_MODE_NOTIFY_BIT;

   // Determine steps and update period based on cycle rate
   const uint16_t rate = GET_VALUE(ch_index, param, TARGET_RATE);
   if (mode != TARGET_MODE_DISABLED && rate != 0) {
      parameter_data_t* p = &channels[ch_index].parameters[param];

      const int8_t previous_step = p->step;

      p->step = 1; // Start at 1 step for highest resolution

      const uint16_t min = GET_VALUE(ch_index, param, TARGET_MIN);
      const uint16_t max = GET_VALUE(ch_index, param, TARGET_MAX);
      while (p->step < 100) {
         uint32_t delta = (max - min) / p->step;

         if (delta == 0) { // If value range is zero, soft disable stepping
            p->step = 0;
            break;
         }

         // Number of microseconds it takes to go from one extent to another
         // rate is currently in millihertz, making the max rate be ~65 Hz
         uint32_t period = 1000000000 / rate;

         if (period >= delta) {                   // Delay is 1us or greater
            p->update_period_us = period / delta; // Number of microseconds between each value
            break;
         } else {
            p->step++; // Increase step so update period will be 1us or more
         }
      }

      // Invert step direction if MODE_DOWN or out of sync
      if (mode == TARGET_MODE_DOWN_RESET || mode == TARGET_MODE_DOWN || (mode == TARGET_MODE_DOWN_UP && previous_step > 0) ||
          (mode == TARGET_MODE_UP_DOWN && previous_step < 0))
         p->step = -(p->step);
   }
}