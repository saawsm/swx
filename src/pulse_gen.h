#ifndef _PULSE_GEN_H
#define _PULSE_GEN_H

#include "swx.h"
#include "parameter.h"
#include "state.h"

#define GET_VALUE(ch_index, param, target) get_state16(REG_CHnn_PARAM + PARAM_TARGET_INDEX((ch_index), (param), (target)))
#define SET_VALUE(ch_index, param, target, value) set_state16(REG_CHnn_PARAM + PARAM_TARGET_INDEX((ch_index), (param), (target)), (value))

typedef struct {
   int8_t step;

   uint32_t next_update_time_us;
   uint32_t update_period_us;
} parameter_data_t;

typedef struct {
   uint8_t state_index;         // The current "waveform" state (e.g. off, on_ramp, on)

   uint32_t last_power_time_us; // The absolute timestamp since the last power update occurred
   uint32_t last_pulse_time_us; // The absolute timestamp since the last pulse occurred

   uint32_t next_state_time_us; // The absolute timestamp for the scheduled next "waveform" state change (e.g. off -> on_ramp -> on)
   uint32_t next_pulse_time_us; // The absolute timestamp for the scheduled next pulse

   parameter_data_t parameters[TOTAL_PARAMS];
} channel_data_t;

void pulse_gen_init();

void pulse_gen_process();

void parameter_update(uint8_t ch_index, param_t param);

#endif // _PULSE_GEN_H