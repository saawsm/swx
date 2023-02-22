#include "util.h"

#include <stdio.h>

#include <pico/stdlib.h>
#include <pico/mutex.h>

#ifndef I2C_MUTEX_TIMEOUT
   #define I2C_MUTEX_TIMEOUT (5000)
#endif

auto_init_mutex(mutex_i2c);

void i2c_scan(i2c_inst_t* i2c) {
   printf("Scanning for I2C devices...\n");
   printf("00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");

   for (uint8_t address = 0; address <= 0x7F; address++) {
      if (address % 0x10 == 0)
         printf("%02x ", address);

      // Perform one byte dummy read using the address.
      // If a slave acknowledges, the number of bytes transferred is returned.
      // If the address is ignored, the function returns -1.

      int ret;
      uint8_t data;
      if ((address & 0x78) == 0 || (address & 0x78) == 0x78) // I2C reserves some addresses for special purposes. These are 1111XXX and 0000XXX.
         ret = PICO_ERROR_GENERIC;
      else
         ret = i2c_read_blocking(i2c, address, &data, 1, false);

      printf(ret < 0 ? "." : "@");
      printf(address % 0x10 == 0x0F ? "\n" : "  ");
   }

   printf("Done.\n");
}

void init_gpio(uint32_t pin, bool out, bool value) {
   gpio_init(pin);
   gpio_set_dir(pin, out);
   gpio_put(pin, value);
}

void blink_led_infinite_loop(uint32_t pin, uint32_t delay_ms) {
   bool state = !gpio_get(pin);
   while (true) {
      gpio_put(pin, state);
      sleep_ms(delay_ms);
      state = !state;
   }
}

int i2c_write(i2c_inst_t* i2c, uint8_t addr, const uint8_t* src, size_t len, bool nostop, uint timeout_us) {
   if (!mutex_enter_timeout_us(&mutex_i2c, I2C_MUTEX_TIMEOUT)) {
      printf("i2c_write: mutex timeout\n");
      return -1;
   }

   int ret = i2c_write_timeout_us(i2c, addr, src, len, nostop, timeout_us);

   mutex_exit(&mutex_i2c);
   return ret;
}

int i2c_read(i2c_inst_t* i2c, uint8_t addr, uint8_t* dst, size_t len, bool nostop, uint timeout_us) {
   if (!mutex_enter_timeout_us(&mutex_i2c, I2C_MUTEX_TIMEOUT)) {
      printf("i2c_read: mutex timeout\n");
      return -1;
   }

   int ret = i2c_read_timeout_us(i2c, addr, dst, len, nostop, timeout_us);

   mutex_exit(&mutex_i2c);
   return ret;
}
