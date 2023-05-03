#ifndef _PARAMETERS_H
#define _PARAMETERS_H

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

typedef struct {
   uint16_t values[TOTAL_TARGETS];

   int8_t step;
   uint32_t next_update_time_us;
   uint32_t update_period_us;

} parameter_t;

void parameter_set(parameter_t* parameter, target_t target, uint16_t value);

#endif // _PARAMETERS_H