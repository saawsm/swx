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
#ifndef _CHANNEL_H
#define _CHANNEL_H

#include <inttypes.h>

#define TOTAL_ANALOG_CHANNELS (3)

typedef enum {
   CHANNEL_INVALID = 0,
   CHANNEL_FAULT,
   CHANNEL_UNCALIBRATED,
   CHANNEL_CALIBRATING,
   CHANNEL_READY,
} channel_status_t;

typedef enum {
   AUDIO_CHANNEL_NONE = 0,

   AUDIO_CHANNEL_MIC,
   AUDIO_CHANNEL_LEFT,
   AUDIO_CHANNEL_RIGHT,
} analog_channel_t;

#endif // _CHANNEL_H