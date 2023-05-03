#include "output.h"
#include "defaults.h"

#include <pico/util/queue.h>

#include "util/i2c.h"
#include "util/gpio.h"

queue_t pulse_queue;
queue_t power_queue;

static channel_t channels[CHANNEL_COUNT] = {
    CH(PIN_CH1_GA, PIN_CH1_GB, CH1_DAC_CHANNEL, CH1_ADC_CHANNEL, pio0, 0, CH1_CAL_THRESHOLD_OK, CH1_CAL_THRESHOLD_OVER, CH1_CAL_OFFSET),
#if CHANNEL_COUNT > 1
    CH(PIN_CH2_GA, PIN_CH2_GB, CH2_DAC_CHANNEL, CH2_ADC_CHANNEL, pio0, 1, CH2_CAL_THRESHOLD_OK, CH2_CAL_THRESHOLD_OVER, CH2_CAL_OFFSET),
#if CHANNEL_COUNT > 2
    CH(PIN_CH3_GA, PIN_CH3_GB, CH3_DAC_CHANNEL, CH3_ADC_CHANNEL, pio0, 2, CH3_CAL_THRESHOLD_OK, CH3_CAL_THRESHOLD_OVER, CH3_CAL_OFFSET),
#if CHANNEL_COUNT > 3
    CH(PIN_CH4_GA, PIN_CH4_GB, CH4_DAC_CHANNEL, CH4_ADC_CHANNEL, pio0, 3, CH4_CAL_THRESHOLD_OK, CH4_CAL_THRESHOLD_OVER, CH4_CAL_OFFSET),
#endif
#endif
#endif
};

static pulse_t pulse;           // current pulse
static bool fetch_pulse = true; // if true, fetch next pulse from pulse_queue

void output_init() {
   LOG_INFO("Init output...\n");

// Assume PSU always on if pin is undefined.
#ifdef PIN_REG_EN
   init_gpio(PIN_REG_EN, GPIO_OUT, 0);
#endif
   set_power_enabled(false);

   queue_init(&pulse_queue, sizeof(pulse_t), 10);

   // Print I2C devices found on bus
   i2c_scan(I2C_PORT); // TODO: Validate missing I2C devices

   for (uint8_t index = 0; index < CHANNEL_COUNT; index++)
      channel_init(&channels[index]);

   fetch_pulse = true;
}

void output_free() {
   LOG_DEBUG("Freeing output...\n");
   set_power_enabled(false);

   queue_free(&pulse_queue);
   queue_free(&power_queue);

   for (uint8_t index = 0; index < CHANNEL_COUNT; index++)
      channel_free(&channels[index]);
}

bool output_calibrate_all() {
   LOG_INFO("Starting calibration of all enabled channels...\n");

   // Clear power cmd queue
   pwr_cmd_t cmd;
   while (queue_try_remove(&power_queue, &cmd))
      tight_loop_contents();

   const bool powerWasOn = is_power_enabled();
   set_power_enabled(true);

   bool success = true;
   for (uint8_t index = 0; index < CHANNEL_COUNT; index++) {
      if (((CH_ENABLED >> index) & 1) == 0) // Skip calibration for disabled channel slots
         continue;
      success &= (channel_calibrate(&channels[index]) == CHANNEL_READY);
   }

   set_power_enabled(success && powerWasOn);

   if (success) {
      LOG_INFO("Calibration successful!\n");
   } else {
      LOG_WARN("Calibration failed for one or more channels! Disabled power...\n");
   }

   return success;
}

void output_process_pulses() {
   if (fetch_pulse) {
      if (queue_try_remove(&pulse_queue, &pulse)) {
         if (pulse.abs_time_us < time_us_32() + 1000000ul) // ignore pulses with wait times above 1 second
            fetch_pulse = false;
      }
   } else if (pulse.abs_time_us > time_us_32()) {
      channel_pulse(&channels[pulse.channel], pulse.pos_us, pulse.neg_us);
      fetch_pulse = true;
   }
}

void output_process_power() {
   pwr_cmd_t cmd;
   if (queue_try_remove(&power_queue, &cmd))
      channel_set_power(&channels[cmd.channel], cmd.power);
}

bool output_pulse(uint8_t channel, uint16_t pos_us, uint16_t neg_us, uint32_t abs_time_us) {
   pulse_t pulse = {
       .channel = channel,
       .pos_us = pos_us,
       .neg_us = neg_us,
       .abs_time_us = abs_time_us,
   };
   return queue_try_add(&pulse_queue, &pulse);
}

void output_set_power(uint8_t channel, uint16_t power) {
   pwr_cmd_t cmd = {.channel = channel, .power = power};
   queue_try_add(&power_queue, &cmd);
}

void output_set_gen_enabled(uint8_t channel, bool enabled, uint16_t turn_off_delay_ms) {
   channel_set_gen_enabled(&channels[channel], enabled, turn_off_delay_ms);
}

channel_status_t output_status(uint8_t channel) {
   return channels[channel].status;
}

bool output_gen_enabled(uint8_t channel) {
   return channels[channel].gen_enabled;
}

void set_power_enabled(bool enabled) {
#ifdef PIN_REG_EN
   const bool oldState = is_power_enabled();

   gpio_put(PIN_REG_EN, enabled);

   if (oldState != enabled) {
      if (enabled) {
         LOG_DEBUG("Enabling power...\n");
         sleep_ms(100); // wait for reg to stabilize
      } else {
         LOG_DEBUG("Disabling power...\n");
      }
   }
#endif
}

bool is_power_enabled() {
#ifdef PIN_REG_EN
   return gpio_get(PIN_REG_EN);
#else
   return true;
#endif
}