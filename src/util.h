#ifndef _UTIL_H
#define _UTIL_H

#include "swx.h"

#include <inttypes.h>

#include <hardware/i2c.h>

void i2c_scan();

void init_gpio(uint32_t pin, bool out, bool value);

void blink_led_infinite_loop(uint32_t pin, uint32_t delay_ms);

int i2c_write(i2c_inst_t* i2c, uint8_t addr, const uint8_t* src, size_t len, bool nostop, uint timeout_us);
int i2c_read(i2c_inst_t* i2c, uint8_t addr, uint8_t* dst, size_t len, bool nostop, uint timeout_us);

#endif // _UTIL_H