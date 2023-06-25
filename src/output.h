#ifndef _OUTPUT_H
#define _OUTPUT_H

#include "swx.h"

#define CHANNEL_POWER_MAX (1000)

#ifndef CHANNEL_COUNT
#define CHANNEL_COUNT (4)
#endif

#ifndef CH_CAL_ENABLED
#define CH_CAL_ENABLED (0xff) // Channel mask: channels to calibrate
#endif

void output_init();
void output_free();

bool output_calibrate_all();

void output_process_pulses();
void output_process_power();

bool output_pulse(uint8_t index, uint16_t pos_us, uint16_t neg_us, uint32_t abs_time_us);
void output_set_power(uint8_t index, uint16_t power);

void output_set_gen_enabled(uint8_t index, bool enabled, uint16_t turn_off_delay_ms);

void set_psu_enabled(bool enabled);
bool is_psu_enabled();

#endif // _OUTPUT_H