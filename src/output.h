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

#ifdef CH_CAL_THRESHOLD_OK
#ifndef CH1_CAL_THRESHOLD_OK
#define CH1_CAL_THRESHOLD_OK (CH_CAL_THRESHOLD_OK)
#endif
#ifndef CH2_CAL_THRESHOLD_OK
#define CH2_CAL_THRESHOLD_OK (CH_CAL_THRESHOLD_OK)
#endif
#ifndef CH3_CAL_THRESHOLD_OK
#define CH3_CAL_THRESHOLD_OK (CH_CAL_THRESHOLD_OK)
#endif
#ifndef CH4_CAL_THRESHOLD_OK
#define CH4_CAL_THRESHOLD_OK (CH_CAL_THRESHOLD_OK)
#endif
#endif

#ifdef CH_CAL_THRESHOLD_OVER
#ifndef CH1_CAL_THRESHOLD_OVER
#define CH1_CAL_THRESHOLD_OVER (CH_CAL_THRESHOLD_OVER)
#endif
#ifndef CH2_CAL_THRESHOLD_OVER
#define CH2_CAL_THRESHOLD_OVER (CH_CAL_THRESHOLD_OVER)
#endif
#ifndef CH3_CAL_THRESHOLD_OVER
#define CH3_CAL_THRESHOLD_OVER (CH_CAL_THRESHOLD_OVER)
#endif
#ifndef CH4_CAL_THRESHOLD_OVER
#define CH4_CAL_THRESHOLD_OVER (CH_CAL_THRESHOLD_OVER)
#endif
#endif

#ifdef CH_CAL_OFFSET
#ifndef CH1_CAL_OFFSET
#define CH1_CAL_OFFSET (CH_CAL_OFFSET)
#endif
#ifndef CH2_CAL_OFFSET
#define CH2_CAL_OFFSET (CH_CAL_OFFSET)
#endif
#ifndef CH3_CAL_OFFSET
#define CH3_CAL_OFFSET (CH_CAL_OFFSET)
#endif
#ifndef CH4_CAL_OFFSET
#define CH4_CAL_OFFSET (CH_CAL_OFFSET)
#endif
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