#include "swx.h"

#include <pico/stdlib.h>
#include <pico/multicore.h>

#include <hardware/adc.h>
#include <hardware/i2c.h>
#include <hardware/spi.h>

#include "util.h"

#include "hardware/mcp4728.h"
#include "hardware/ads1015.h"
#include "queued_dac.h"

#include "comms/cli_hardware.h"
#include "protocol.h"

#include "output.h"
#include "pulse_func_gen.h"

using namespace swx;

static QueuedDac* dac;
static Protocol* protocol;

void core1_entry();
void parse(Cli& cli, uint8_t ctrl);

int main() {
// Before waiting for stdio, init GPIO first to ensure PSU is off.
// Assume PSU always on if pin is undefined.
#ifdef PIN_REG_EN
   init_gpio(PIN_REG_EN, GPIO_OUT, 0);
#endif
#ifdef PIN_LED
   init_gpio(PIN_LED, GPIO_OUT, 1);  // turn on led during init
#endif
#ifdef PIN_LED_OK
   init_gpio(PIN_LED_OK, GPIO_OUT, 0);
#endif

   set_sys_clock_khz(250000, false);  // try set clock to 250MHz
   stdio_init_all();

   adc_init();

   // Setup I2C as master for comms with DAC/ADC
   i2c_init(I2C_PORT, I2C_FREQ);
   gpio_set_function(PIN_I2C_SDA, GPIO_FUNC_I2C);
   gpio_set_function(PIN_I2C_SCL, GPIO_FUNC_I2C);
   gpio_pull_up(PIN_I2C_SDA);
   gpio_pull_up(PIN_I2C_SCL);

   // Setup SPI as slave for comms with host device
   spi_init(SPI_PORT, SPI_FREQ);
   spi_set_format(SPI_PORT, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
   spi_set_slave(SPI_PORT, true);
   gpio_set_function(PIN_SPI_MOSI, GPIO_FUNC_SPI);
   gpio_set_function(PIN_SPI_SCK, GPIO_FUNC_SPI);
   gpio_set_function(PIN_SPI_MISO, GPIO_FUNC_SPI);
   gpio_set_function(PIN_SPI_CS, GPIO_FUNC_SPI);

   // Print I2C devices found on bus
   i2c_scan();

   Adc* adc = new ADS1015(I2C_PORT, ADC_ADDRESS, ADS1015::GAIN_ONE, ADS1015::RATE_3300SPS);
   dac = new QueuedDac(new MCP4728(I2C_PORT, DAC_ADDRESS));  // queued dac for processing on core1, since setting each dac channel takes ~110us

   // Startup core1 before output calibration, since channel calibration uses dac
   multicore_reset_core1();
   multicore_launch_core1(core1_entry);

   // Init output channels
   Output::init(pio0, *adc, *dac);

   Output& output = Output::instance();

   output.setPowerEnabled(true);  // enable channel power supply
   if (!output.calibrateAll()) {  // calibrate all enabled channels, if failed blink led
#ifdef PIN_LED_OK
      blink_led_infinite_loop(PIN_LED_OK, 250);
#else
      while (true) tight_loop_contents();
#endif
   }

   PulseFuncGenerator funcGen(output);

   // Init CLI for stdin/out and spi
   // Commands can be parsed from STDIO, but receiving responses may be mixed with debugging information (depending on config of pico_enable_stdio).
   // Responses will all start with 0xF5 (RES_START).
   // Cli* cli = new Cli(parse);
   Cli* cliSpi = new SpiCli(parse, SPI_PORT);

   // Init protocol for processing commands from Cli
   protocol = new Protocol(output, funcGen);

#ifdef PIN_LED
   gpio_put(PIN_LED, 0);  // init finished, turn off led
#endif

   printf("Starting core0 loop...\n");
   while (true) {
      // cli->process();
      cliSpi->process();

      funcGen.process();
      output.process();
   }
}

void core1_entry() {
   printf("Starting core1 loop...\n");
   while (true) {
      dac->process();  // process queued dac commands
   }
}

void parse(Cli& cli, uint8_t ctrl) {
   protocol->parseCommand(cli, ctrl);
}