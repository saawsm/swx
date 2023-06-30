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
#include "i2c.h"

#include <pico/mutex.h>

#ifdef I2C_MUTEX_TIMEOUT
auto_init_mutex(mutex_i2c);
#endif

void i2c_scan(i2c_inst_t* i2c) { // TODO: Return found I2C device addresses
   LOG_DEBUG("Scanning I2C devices...\n");

#ifdef I2C_MUTEX_TIMEOUT
   if (!mutex_enter_timeout_us(&mutex_i2c, I2C_MUTEX_TIMEOUT)) {
      LOG_WARN("i2c_scan: Mutex timeout!\n");
      return -1;
   }
#endif

   for (uint8_t address = 0; address <= 0x7F; address++) {
      // Perform one byte dummy read using the address.
      // If a slave acknowledges, the number of bytes transferred is returned.
      // If the address is ignored, the function returns -2.
      uint8_t data;
      if ((address & 0x78) == 0 || (address & 0x78) == 0x78) // I2C reserves some addresses for special purposes. These are 1111XXX and 0000XXX.
         continue;

      if (i2c_read_blocking(i2c, address, &data, 1, false) > 0)
         LOG_DEBUG("Found device at 0x%02x\n", address);
   }

#ifdef I2C_MUTEX_TIMEOUT
   mutex_exit(&mutex_i2c);
#endif

   LOG_DEBUG("Done.\n");
}

int i2c_write(i2c_inst_t* i2c, uint8_t addr, const uint8_t* src, size_t len, bool nostop, uint timeout_us) {
#ifdef I2C_MUTEX_TIMEOUT
   if (!mutex_enter_timeout_us(&mutex_i2c, I2C_MUTEX_TIMEOUT)) {
      LOG_WARN("i2c_write: addr=%u - Mutex timeout!\n", addr);
      return -1;
   }
#endif

   int ret = i2c_write_timeout_us(i2c, addr, src, len, nostop, timeout_us);

#ifdef I2C_MUTEX_TIMEOUT
   mutex_exit(&mutex_i2c);
#endif
   return ret;
}

int i2c_read(i2c_inst_t* i2c, uint8_t addr, uint8_t* dst, size_t len, bool nostop, uint timeout_us) {
#ifdef I2C_MUTEX_TIMEOUT
   if (!mutex_enter_timeout_us(&mutex_i2c, I2C_MUTEX_TIMEOUT)) {
      LOG_WARN("i2c_read: addr=%u - Mutex timeout!\n", addr);
      return -1;
   }
#endif

   int ret = i2c_read_timeout_us(i2c, addr, dst, len, nostop, timeout_us);

#ifdef I2C_MUTEX_TIMEOUT
   mutex_exit(&mutex_i2c);
#endif
   return ret;
}
