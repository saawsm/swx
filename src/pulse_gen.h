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
#ifndef _PULSE_GEN_H
#define _PULSE_GEN_H

#include "swx.h"
#include "parameter.h"
#include "state.h"

// Returns the uint16 parameter+target value for the given channel. See param_t and target_t.
#define GET_VALUE(ch_index, param, target) get_state16(REG_CHn_PARAM_w + PARAM_TARGET_INDEX((ch_index), (param), (target)))

// Sets the uint16 parameter target value for the given channel. See param_t and target_t.
#define SET_VALUE(ch_index, param, target, value) set_state16(REG_CHn_PARAM_w + PARAM_TARGET_INDEX((ch_index), (param), (target)), (value))

typedef struct {
   int8_t step; // number of steps to increment/decrement per parameter update

   uint32_t next_update_time_us; // the next parameter step time in microseconds
   uint32_t update_period_us;    // parameter step update period in microseconds
} parameter_data_t;

typedef struct {
   uint8_t state_index; // The current "waveform" state (e.g. off, on_ramp, on)

   uint32_t last_power_time_us; // The absolute timestamp since the last power update occurred
   uint32_t last_pulse_time_us; // The absolute timestamp since the last pulse occurred

   uint32_t next_state_time_us; // The absolute timestamp for the scheduled next "waveform" state change (e.g. off -> on_ramp -> on)
   uint32_t next_pulse_time_us; // The absolute timestamp for the scheduled next pulse

   parameter_data_t parameters[TOTAL_PARAMS];
} channel_data_t;

void pulse_gen_init();

// Update pulse generator by updating sequencer, parameters, power level transitions, and generating the actual
// pulses manually or via audio processing depending on configured source.
void pulse_gen_process();

// Updates the parameter step period and step size based on the current target mode, minmum, maximum, and rate.
// Should be called whenever TARGET_MODE, TARGET_MIN, TARGET_MAX, or TARGET_RATE is changed, and the parameter is
// sweeping the value.
void parameter_update(uint8_t ch_index, param_t param);

// execute each action between indices al_start and al_end
void execute_action_list(uint8_t al_start, uint8_t al_end);

#endif // _PULSE_GEN_H