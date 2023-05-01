#include "swx.h"

#include <pico/multicore.h>

#include <hardware/adc.h>
#include <hardware/i2c.h>
#include <hardware/spi.h>

#include "util/i2c.h"
#include "util/gpio.h"

static void init() {
// Before waiting for stdio to init, configure GPIO first to ensure PSU is off.
// Assume PSU always on if pin is undefined.
#ifdef PIN_REG_EN
   init_gpio(PIN_REG_EN, GPIO_OUT, 0);
#endif
#ifdef PIN_LED
   init_gpio(PIN_LED, GPIO_OUT, 1); // turn on led during init
#endif

   bool clk_success = set_sys_clock_khz(250000, false); // try set clock to 250MHz
   stdio_init_all();                                    // needs to be called after setting clock

   LOG_INFO("~~ swx driver %u ~~\n", SWX_VERSION);
   if (clk_success)
      LOG_DEBUG("sys_clk set to 250MHz\n");

   LOG_DEBUG("Init internal ADC...\n");
   adc_init();

   // Setup I2C as master for comms with DAC (and optionally an ADC)
   LOG_DEBUG("Init I2C...\n");
   i2c_init(I2C_PORT, I2C_FREQ);
   gpio_set_function(PIN_I2C_SDA, GPIO_FUNC_I2C);
   gpio_set_function(PIN_I2C_SCL, GPIO_FUNC_I2C);
   gpio_pull_up(PIN_I2C_SDA);
   gpio_pull_up(PIN_I2C_SCL);

   // Setup SPI as slave for comms with host device
   LOG_DEBUG("Init SPI...\n");
   spi_init(SPI_PORT, SPI_FREQ);
   spi_set_format(SPI_PORT, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
   spi_set_slave(SPI_PORT, true);
   gpio_set_function(PIN_SPI_MOSI, GPIO_FUNC_SPI);
   gpio_set_function(PIN_SPI_SCK, GPIO_FUNC_SPI);
   gpio_set_function(PIN_SPI_MISO, GPIO_FUNC_SPI);
   gpio_set_function(PIN_SPI_CS, GPIO_FUNC_SPI);
}

int main() {
   init();

   // Print I2C devices found on bus
   i2c_scan(I2C_PORT); // TODO: Validate missing I2C devices
}