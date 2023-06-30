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

bool output_calibrate_all();

void output_process_pulses();
void output_process_power();

bool output_pulse(uint8_t index, uint16_t pos_us, uint16_t neg_us, uint32_t abs_time_us);
void output_set_power(uint8_t index, uint16_t power);

void set_psu_enabled(bool enabled);
bool is_psu_enabled();

#endif // _OUTPUT_H