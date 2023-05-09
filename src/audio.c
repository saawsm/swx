#include "swx.h"
#include <stdlib.h>

#include "analog_capture.h"
#include "output.h"

static void process_samples(channel_t* ch, uint8_t channel, uint16_t sample_count, uint8_t* buffer, float* out_intensity, uint32_t capture_end_time_us);
static void process_sample(channel_t* ch, uint8_t channel, uint32_t time_us, int16_t value);

static int16_t last_sample_values[CHANNEL_COUNT] = {0};
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
void audio_process(channel_t* ch, uint8_t channel, uint16_t power) {
   uint16_t sample_count;
   uint8_t* sample_buffer;
   uint32_t capture_end_time_us = 0;

   // Fetch audio from the specific analog channel
   fetch_analog_buffer(ch->audio_src, &sample_count, &sample_buffer, &capture_end_time_us);

   // Skip processing if audio samples are older than previously processed
   if (capture_end_time_us <= last_process_times_us[channel])
      return;
   last_process_times_us[channel] = capture_end_time_us;

   // Process audio samples by converting them into pulses and calculating intensity to scale power level
   float intensity;
   process_samples(ch, channel, sample_count, sample_buffer, &intensity, capture_end_time_us);

   // Set channel output power, limit updates to ~2.2 kHz since it takes the DAC about ~110us/ch
   uint32_t time = time_us_32();
   if (time - ch->last_power_time_us > 110 * CHANNEL_COUNT) {
      ch->last_power_time_us = time;

      const uint16_t power_max = ch->parameters[PARAM_POWER].values[TARGET_MAX];

      // scale power level with audio intensity, ensure power is between min-max
      power *= intensity;
      if (power > power_max)
         power = power_max;

      output_set_power(channel, power);
   }
}

static void process_samples(channel_t* ch, uint8_t channel, uint16_t sample_count, uint8_t* buffer, float* out_intensity, uint32_t capture_end_time_us) {
   // Find the min, max, and mean sample values
   uint16_t min = UINT16_MAX;
   uint16_t max = 0;
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
   if (abs(min - avg) < 7 && abs(max - avg) < 7) {
      *out_intensity = 0;
      return;
   }

   *out_intensity = (max - min) / 255.0f;                                            // crude approximation of volume

   const uint32_t capture_duration_us = get_capture_duration_us(ch->audio_src);      // buffer capture duration
   const uint32_t capture_start_time_us = capture_end_time_us - capture_duration_us; // time when capture started

   const uint32_t sample_duration_us = capture_duration_us / sample_count;           // single sample duration

   // Call process each sample at roughly the time it happened, with a signed value ranging from -128 to +128 (assuming DC offset is ~1.65V)
   for (uint16_t i = 0; i < sample_count; i++) {
      const uint32_t sample_time_us = capture_start_time_us + (sample_duration_us * i);
      const int16_t value = avg - buffer[i];

      process_sample(ch, channel, sample_time_us, value);
   }
}

static inline void process_sample(channel_t* ch, uint8_t channel, uint32_t time_us, int16_t value) {
   // Check for zero crossing
   if (((value > 0 && last_sample_values[channel] <= 0) || (value < 0 && last_sample_values[channel] >= 0))) {

      uint32_t min_period = 10000000ul / ch->parameters[PARAM_FREQUENCY].values[TARGET_MAX]; // dHz -> us
      if (min_period < 2000)                                                                 // clamp to 500 Hz
         min_period = 2000;

      if (time_us - ch->last_pulse_time_us >= min_period) { // limit pulses to parameter frequency maximum or 500 Hz, whatever is lower
         ch->last_pulse_time_us = time_us;

         const uint16_t pulse_width = ch->parameters[PARAM_PULSE_WIDTH].values[TARGET_VALUE];

         output_pulse(channel, pulse_width, pulse_width, time_us + 20000); // 20 ms in future
      }
   }

   last_sample_values[channel] = value;
}