#include "protocol.h"
#include "state.h"

#include <pico/i2c_slave.h>

uint8_t mem[256];

static struct {
   uint8_t address;
   bool address_written;
} ctx;

static void __not_in_flash_func(i2c_slave_handler)(i2c_inst_t* i2c, i2c_slave_event_t event) {
   switch (event) {
      case I2C_SLAVE_RECEIVE: // master has written some data
         if (!ctx.address_written) {
            // writes always start with the memory address
            ctx.address = i2c_read_byte_raw(i2c);
            ctx.address_written = true;
         } else {
            // save into memory
            mem[ctx.address] = i2c_read_byte_raw(i2c);
            ctx.address++;
         }
         break;
      case I2C_SLAVE_REQUEST: // master is requesting data
         // load from memory
         i2c_write_byte_raw(i2c, mem[ctx.address]);
         ctx.address++;
         break;
      case I2C_SLAVE_FINISH: // master has signalled Stop / Restart
         ctx.address_written = false;
         break;
      default:
         break;
   }
}

void protocol_init() {
   LOG_DEBUG("Init protocol...\n");
   i2c_slave_init(I2C_PORT_COMMS, I2C_ADDRESS_COMMS, i2c_slave_handler);
}

void protocol_free() {
   i2c_slave_deinit(I2C_PORT_COMMS);
}

void protocol_process() {}