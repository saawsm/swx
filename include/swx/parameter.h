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
#ifndef _PARAMETER_H
#define _PARAMETER_H

#include <inttypes.h>

typedef enum {
   /// The intensity of the signal as a percent (0 to 1000).
   PARAM_POWER = 0,

   /// The frequency of pulses generated in dHz (decihertz, 1 Hz = 10 dHz)
   PARAM_FREQUENCY,

   /// The duration of each pulse (0 us to 500 us).
   PARAM_PULSE_WIDTH,

   /// The number of milliseconds the output is on.
   PARAM_ON_TIME,

   /// The duration in milliseconds to smoothly ramp intensity level when going from off to on. `_|‾`
   PARAM_ON_RAMP_TIME,

   /// The number of milliseconds the output is off.
   PARAM_OFF_TIME,

   /// The duration in milliseconds to smoothly ramp intensity level when going from on to off. `‾|_`
   PARAM_OFF_RAMP_TIME,

   PARAM_RESERVED_NOT_USED_1,

   PARAM_RESERVED_NOT_USED_2,

   TOTAL_PARAMS // Number of parameters in enum, used for arrays
} param_t;

// The notify bit flag settable within the TARGET_MODE value. Asserts notify GPIO and updates parameter flags.
#define TARGET_MODE_NOTIFY_BIT (1 << 15)

typedef enum {
   /// The actual parameter value.
   TARGET_VALUE = 0,

   /// The minimum the parameter value can be for parameter cycling.
   TARGET_MIN,

   /// The maximum the parameter value can be for parameter cycling.
   TARGET_MAX,

   /// The frequency of parameter cycling in mHz (millihertz, 1 Hz = 1000 mHz)
   TARGET_RATE,

   /// The cycling mode the parameter is using. Determines how the value resets when it reaches min/max.
   TARGET_MODE,

   /// Execute actions between start/end indices when value reaches min/max. Lower byte contains end index, upper byte contains start index.
   /// Disable by setting upper and lower bytes equal
   TARGET_ACTION_RANGE,

   TOTAL_TARGETS // Number of targets in enum, used for arrays

} target_t;

typedef enum {
   TARGET_MODE_DISABLED = 0,

   /// Ramp smoothly between minimum and maximum (defaults to incrementing)
   TARGET_MODE_UP_DOWN,

   // Ramp smoothly between maximum and minimum (same as UP_DOWN, but starts with decrementing)
   TARGET_MODE_DOWN_UP,

   /// Ramp smoothly from minimum to maximum and then reset at minimum again.
   TARGET_MODE_UP_RESET,

   /// Ramp smoothly from maximum to minimum and then reset at maximum again.
   TARGET_MODE_DOWN_RESET,

   /// Ramp smoothly from minimum to maximum and then disable cycling.
   TARGET_MODE_UP,

   /// Ramp smoothly from maximum to minimum and then disable cycling.
   TARGET_MODE_DOWN,

   TARGET_MODE_RESERVED_NOT_USED_1

} target_mode_t;

typedef enum {
   ACTION_NONE = 0,
   ACTION_SET,
   ACTION_INCREMENT,
   ACTION_DECREMENT,
   ACTION_ENABLE,
   ACTION_DISABLE,
   ACTION_TOGGLE,
   ACTION_EXECUTE,
   ACTION_PARAM_UPDATE,
} action_type_t;

#define PARAM_TARGET_INDEX_OFFSET(param, target) ((((param)*2) * TOTAL_TARGETS) + ((target)*2))
#define PARAM_TARGET_INDEX_TOTAL (PARAM_TARGET_INDEX_OFFSET(TOTAL_PARAMS - 1, TOTAL_TARGETS - 1))

// Calculates the byte offset for the uint16_t value representing a parameter target for the given channel.
// Assumes values are 16-bit (indices are spaced by 2)
#define PARAM_TARGET_INDEX(ch_index, param, target) (((ch_index)*PARAM_TARGET_INDEX_TOTAL) + PARAM_TARGET_INDEX_OFFSET(param, target) + ((ch_index)*2))

// Array size in bytes for all parameter target indices
#define PARAM_TARGET_INDEX_MAX(channel_count) (PARAM_TARGET_INDEX((channel_count)-1, TOTAL_PARAMS - 1, TOTAL_TARGETS - 1) + 2)

#endif // _PARAMETER_H