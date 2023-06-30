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
#ifndef _ANALOG_CAPTURE_H
#define _ANALOG_CAPTURE_H

#include "swx.h"

#include "channel.h"

void analog_capture_init();

void analog_capture_start();
void analog_capture_stop();

bool fetch_analog_buffer(analog_channel_t channel, uint16_t* samples, uint16_t** buffer, uint32_t* capture_end_time_us);

uint32_t get_capture_duration_us(analog_channel_t channel);

#endif // _ANALOG_CAPTURE_H