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
/*
 * Based on https://github.com/CrashOverride85/zc95/blob/c67a58668be187b63eeebab66ec8583b33494d43/source/zc95/AudioInput/CAudio3Process.cpp
 * Changes: Add support for swx channel parameter system. Allow adjustment of the pulse width, maximum power, and frequency.
 */
#include "swx.h"
#include <stdlib.h>

#include "pulse_gen.h"

#include "analog_capture.h"
#include "output.h"

static inline void process_samples(channel_data_t* ch, uint8_t ch_index, analog_channel_t audio_src, uint16_t sample_count, uint16_t* buffer, float* out_intensity,
                                   uint32_t capture_end_time_us);
static void process_sample(channel_data_t* ch, uint8_t ch_index, uint32_t time_us, int32_t value);

static int32_t last_sample_values[CHANNEL_COUNT] = {0};
static uint32_t last_process_times_us[CHANNEL_COUNT] = {0};

/*
 * Process audio in a similar way to the Audio3 mode of a mk312b box.
 *
 * - Every zero crossing, trigger a pulse, but limit pulse frequency to the maximum parameter or 500 Hz, whatever is lower.
 *
 * - Adjust output power based on volume. Using the difference between min and max of the sample capture values.
 *
 * - Audio is captured via DMA in blocks of about 341 (1/3 of 1024) samples per analog channel, making the samples at least ~16ms old.
 *
 * - We know fairly accurately when the capture finished, and how long it was. So we can use that information to
 *   schedule pulses +16ms in the future. The downside is that this introduces ~16ms of latency.
 */
void audio_process(channel_data_t* ch, uint8_t ch_index, uint16_t power) {
   uint16_t sample_count;
   uint16_t* sample_buffer;
   uint32_t capture_end_time_us = 0;

   // Fetch audio from the specific analog channel
   const analog_channel_t audio_src = get_state(REG_CHn_AUDIO_SRC + ch_index);
   fetch_analog_buffer(audio_src, &sample_count, &sample_buffer, &capture_end_time_us);

   // Skip processing if audio samples are older than previously processed
   if (capture_end_time_us <= last_process_times_us[ch_index])
      return;
   last_process_times_us[ch_index] = capture_end_time_us;

   // Process audio samples by converting them into pulses and calculating intensity to scale power level
   float intensity;
   process_samples(ch, ch_index, audio_src, sample_count, sample_buffer, &intensity, capture_end_time_us);

   // Set channel output power, limit updates to ~2.2 kHz since it takes the DAC about ~110us/ch
   uint32_t time = time_us_32();
   if (time - ch->last_power_time_us > 110 * CHANNEL_COUNT) {
      ch->last_power_time_us = time;

      const uint16_t power_max = GET_VALUE(ch_index, PARAM_POWER, TARGET_MAX);

      // scale power level with audio intensity, ensure power is between min-max
      power *= intensity;
      if (power > power_max)
         power = power_max;

      output_set_power(ch_index, power);
   }
}

static inline void process_samples(channel_data_t* ch, uint8_t ch_index, analog_channel_t audio_src, uint16_t sample_count, uint16_t* buffer, float* out_intensity,
                                   uint32_t capture_end_time_us) {
   // Find the min, max, and mean sample values
   uint32_t min = UINT32_MAX;
   uint32_t max = 0;
   uint32_t total = 0;
   for (uint16_t x = 0; x < sample_count; x++) {
      if (buffer[x] > max)
         max = buffer[x];

      if (buffer[x] < min)
         min = buffer[x];

      total += buffer[x];
   }

   const uint16_t avg = total / sample_count;

   // Noise filter
   if (abs((int32_t)min - avg) < 7 && abs((int32_t)max - avg) < 7) {
      *out_intensity = 0;
      return;
   }

   *out_intensity = (max - min) / 255.0f;                                            // crude approximation of volume

   const uint32_t capture_duration_us = get_capture_duration_us(audio_src);          // buffer capture duration
   const uint32_t capture_start_time_us = capture_end_time_us - capture_duration_us; // time when capture started

   const uint32_t sample_duration_us = capture_duration_us / sample_count;           // single sample duration

   // Call process each sample at roughly the time it happened, with a signed value ranging from -128 to +128 (assuming DC offset is ~1.65V)
   for (uint16_t i = 0; i < sample_count; i++) {
      const uint32_t sample_time_us = capture_start_time_us + (sample_duration_us * i);
      const int32_t value = avg - buffer[i];

      process_sample(ch, ch_index, sample_time_us, value);
   }
}

static inline void process_sample(channel_data_t* ch, uint8_t ch_index, uint32_t time_us, int32_t value) {
   // Check for zero crossing
   if (((value > 0 && last_sample_values[ch_index] <= 0) || (value < 0 && last_sample_values[ch_index] >= 0))) {

      uint32_t min_period = 10000000ul / GET_VALUE(ch_index, PARAM_FREQUENCY, TARGET_MAX); // dHz -> us
      if (min_period < 2000)                                                               // clamp to 500 Hz
         min_period = 2000;

      if (time_us - ch->last_pulse_time_us >= min_period) { // limit pulses to parameter frequency maximum or 500 Hz, whatever is lower
         ch->last_pulse_time_us = time_us;

         const uint16_t pulse_width = GET_VALUE(ch_index, PARAM_PULSE_WIDTH, TARGET_VALUE);

         output_pulse(ch_index, pulse_width, pulse_width, time_us + 20000); // 20 ms in future
      }
   }

   last_sample_values[ch_index] = value;
}