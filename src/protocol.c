#include "protocol.h"
#include "state.h"

#include <pico/i2c_slave.h>

uint8_t mem[MAX_STATE_MEM_SIZE];

static struct {
   uint16_t address;
   uint8_t ready;
} ctx;

#define CHECK_BOUNDS(n)                                                                                                                                                  \
   do {                                                                                                                                                                  \
      n;                                                                                                                                                                 \
      if (ctx.address >= MAX_STATE_MEM_SIZE)                                                                                                                             \
         ctx.address = 0;                                                                                                                                                \
   } while (0)

__not_in_flash_func(i2c_slave_handler)(i2c_inst_t* i2c, i2c_slave_event_t event) {
   switch (event) {
      case I2C_SLAVE_RECEIVE: // master has written some data
         if (ctx.ready < 2) {
            // writes always start with the memory address
            if (ctx.ready == 0) {
               ctx.address = i2c_read_byte_raw(i2c);
            } else {
               ctx.address |= i2c_read_byte_raw(i2c) << 8;
               CHECK_BOUNDS();
            }
            ctx.ready++;
         } else {
            // save into memory
            const uint8_t value = i2c_read_byte_raw(i2c);
            if (ctx.address >= READ_ONLY_ADDRESS_BOUNDARY) {
               mem[ctx.address] = value;
               CHECK_BOUNDS(ctx.address++);
            }
         }
         break;
      case I2C_SLAVE_REQUEST: // master is requesting data
         // load from memory
         i2c_write_byte_raw(i2c, mem[ctx.address]);
         CHECK_BOUNDS(ctx.address++);
         break;
      case I2C_SLAVE_FINISH: // master has signalled Stop / Restart
         ctx.ready = 0;
         break;
      default:
         break;
   }
}

void protocol_init() {
   memset(mem, 0, MAX_STATE_MEM_SIZE);

   // Set readonly info
   set_state16(REG_VERSION, SWX_VERSION);
   set_state(REG_CHANNEL_COUNT, CHANNEL_COUNT);
   set_state(REG_CH_CAL_ENABLED, CH_CAL_ENABLED);

   LOG_DEBUG("Init protocol...\n");
   i2c_slave_init(I2C_PORT_COMMS, I2C_ADDRESS_COMMS, i2c_slave_handler);
}

void protocol_free() {
   i2c_slave_deinit(I2C_PORT_COMMS);
}

void protocol_process() {}