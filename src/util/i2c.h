#ifndef _I2C_H
#define _I2C_H

#include <hardware/i2c.h>

void i2c_scan(i2c_inst_t* i2c);

int i2c_write(i2c_inst_t* i2c, uint8_t addr, const uint8_t* src, size_t len, bool nostop, uint timeout_us);
int i2c_read(i2c_inst_t* i2c, uint8_t addr, uint8_t* dst, size_t len, bool nostop, uint timeout_us);

#endif // _I2C_H