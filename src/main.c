#include "swx.h"

#include <pico/multicore.h>

#include <hardware/adc.h>
#include <hardware/i2c.h>
#include <hardware/spi.h>

#include "output.h"
#include "pulse_gen.h"
#include "protocol.h"

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

   LOG_INFO("~~ swx driver %u ~~\n", SWX_VERSION);
   if (clk_success)
      LOG_DEBUG("sys_clk set to 250MHz\n");

   LOG_DEBUG("Init internal ADC...\n");
   adc_init();

   // Init external DAC and ADC
   extern void init_dac();
   init_dac();

   extern void init_adc();
   init_adc();

   // Setup I2C as master for comms with DAC (and optionally an ADC)
#ifdef I2C_PORT
   LOG_DEBUG("Init I2C...\n");
   i2c_init(I2C_PORT, I2C_FREQ);
   gpio_set_function(PIN_I2C_SDA, GPIO_FUNC_I2C);
   gpio_set_function(PIN_I2C_SCL, GPIO_FUNC_I2C);
   gpio_pull_up(PIN_I2C_SDA);
   gpio_pull_up(PIN_I2C_SCL);
#endif

   // Setup SPI as slave for comms with host device
#ifdef SPI_PORT
   LOG_DEBUG("Init SPI...\n");
   spi_init(SPI_PORT, SPI_FREQ);
   spi_set_format(SPI_PORT, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
   spi_set_slave(SPI_PORT, true);
   gpio_set_function(PIN_SPI_MOSI, GPIO_FUNC_SPI);
   gpio_set_function(PIN_SPI_SCK, GPIO_FUNC_SPI);
   gpio_set_function(PIN_SPI_MISO, GPIO_FUNC_SPI);
   gpio_set_function(PIN_SPI_CS, GPIO_FUNC_SPI);
#endif
}

int main() {
   // Initialize hardware
   init();

   // Initialize output channels and needed subsystems
   output_init();

   // Calibrate all output channels
   if (!output_calibrate_all()) // blink led at 4 Hz, if calibration failed
      add_repeating_timer_us(HZ_TO_US(4), blink_led_timer_cb, NULL, &failure_timer);

   LOG_INFO("Starting core1...\n");
   multicore_reset_core1();
   multicore_launch_core1(core1_entry);

   LOG_INFO("Starting core0 loop...\n");

   // Device ready. Generate fake received MSG_CMD_STATUS message, so we can reply instead
   // of constructing the byte buffer manually.
   // Note: If SPI master is unconnected, ensure SPI_PORT is undefined or swx will hang
   parse_message(MSG_CH_SPI | MSG_CH_UART | MSG_CH_STDIO, MSG_CMD_STATUS << MSG_CMD);

   while (true) {
      protocol_process(MSG_CH_SPI | MSG_CH_UART | MSG_CH_STDIO);
      pulse_gen_process();
      output_process_pulses();
   }

   // Code execution shouldn't get this far...
   output_free(); // release resources and put GPIO into a safe and known state
}

void core1_entry() {
   LOG_INFO("Starting core1 loop...\n");

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
