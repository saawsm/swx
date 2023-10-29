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
#include "swx.h"

#include <pico/multicore.h>

#include <hardware/adc.h>
#include <hardware/i2c.h>

#include "output.h"
#include "protocol.h"
#include "analog_capture.h"
#include "pulse_gen.h"
#include "trigger.h"

#include "util/i2c.h"
#include "util/gpio.h"

void core1_entry();

bool blink_led_timer_cb(repeating_timer_t* rt);

repeating_timer_t failure_timer;

static void init() {
#ifdef PIN_REG_EN
   init_gpio(PIN_REG_EN, GPIO_OUT, 0); // ensure PSU is disabled while waiting for stdio_init
#endif
#ifdef PIN_LED
   init_gpio(PIN_LED, GPIO_OUT, 1);                     // turn on led during init
#endif
   init_gpio(PIN_INT, GPIO_OUT, 1);                     // active low

   bool clk_success = set_sys_clock_khz(250000, false); // try set clock to 250MHz
   stdio_init_all();                                    // needs to be called after setting clock

   LOG_INFO("~~ swx driver %u ~~\nStarting up...\n", SWX_VERSION);
   if (clk_success) {
      LOG_DEBUG("sys_clk set to 250MHz\n");
   }

   // Setup I2C as slave for comms with control device.
   protocol_init();

   // Setup I2C as master for comms with DAC (and optionally an ADC)
#ifdef I2C_PORT_PERIF
   LOG_DEBUG("Init peripheral I2C...\n");
   i2c_init(I2C_PORT_PERIF, I2C_FREQ_PERIF);
   gpio_set_function(PIN_I2C_SDA_PERIF, GPIO_FUNC_I2C);
   gpio_set_function(PIN_I2C_SCL_PERIF, GPIO_FUNC_I2C);
   gpio_pull_up(PIN_I2C_SDA_PERIF);
   gpio_pull_up(PIN_I2C_SCL_PERIF);
#endif

   // Init external DAC and ADC
   extern void init_dac();
   init_dac();

   extern void init_adc();
   init_adc();

   analog_capture_init();
   analog_capture_start();
}

int main() {
   // Initialize hardware
   init();

   // Initialize output channels and needed subsystems
   output_init();

   // Calibrate all output channels
   if (!output_calibrate_all()) // blink led at 4 Hz, if calibration failed
      add_repeating_timer_us(HZ_TO_US(4), blink_led_timer_cb, NULL, &failure_timer);

   LOG_DEBUG("Starting core1...\n");
   multicore_reset_core1();
   multicore_launch_core1(core1_entry);

   pulse_gen_init();
   
   triggers_init();

   LOG_DEBUG("Starting core0 loop...\n");

   // Device ready. Assert interrupt pin, to notify control device.
   gpio_assert(PIN_INT);
   LOG_INFO("Ready.\n");

   while (true) {
      protocol_process();

      pulse_gen_process();
      output_process_pulses();
      triggers_process();
   }

   // Code execution shouldn't get this far...
   LOG_WARN("End reached! Something went wrong...\n");
}

void core1_entry() {
   LOG_DEBUG("Starting core1 loop...\n");

   while (true) {
      output_process_power(); // process channel set power on core1, since DAC takes ~110us/ch to set
   }
}

bool blink_led_timer_cb(repeating_timer_t* rt) {
   (void)rt;
#ifdef PIN_LED
   gpio_toggle(PIN_LED);
   return true;
#else
   return false;
#endif
}
