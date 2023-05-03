#ifndef _OUTPUT_H
#define _OUTPUT_H

#include "channel.h"

#ifndef CHANNEL_COUNT
#define CHANNEL_COUNT (4)
#endif

typedef struct {
   uint8_t channel;
   uint16_t power;
} pwr_cmd_t;

typedef struct {
   uint32_t abs_time_us;

   uint8_t channel;

   uint16_t pos_us;
   uint16_t neg_us;
} pulse_t;

void output_init();
void output_free();

bool output_calibrate_all();

void output_process_pulses();
void output_process_power();

bool output_pulse(uint8_t channel, uint16_t pos_us, uint16_t neg_us, uint32_t abs_time_us);
void output_set_power(uint8_t channel, uint16_t power);

void output_set_power_level(uint8_t channel, uint16_t power_level);
uint16_t output_power_level(uint8_t channel);

void output_set_gen_enabled(uint8_t channel, bool enabled, uint16_t turn_off_delay_ms);

bool output_gen_enabled(uint8_t channel);
channel_status_t output_status(uint8_t channel);

void set_power_enabled(bool enabled);
bool is_power_enabled();

#endif // _OUTPUT_H