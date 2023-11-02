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
#include "protocol.h"
#include "state.h"

#include "output.h"
#include "pulse_gen.h"

#include <pico/i2c_slave.h>

uint8_t mem[MAX_STATE_MEM_SIZE];    // I2C master accessible state memory
static volatile bool dirty = false; // set true when I2C master sent a STOP or RESTART, set false when processed.

static struct {
   uint16_t address; // the current memory address I2C is writing/reading at.
   uint8_t ready;    // determines what part of the address is being written. 0: lower byte, 1: upper byte, 2: ready
} ctx;

#define CHECK_BOUNDS(n)                                                                                                                                                  \
   do {                                                                                                                                                                  \
      n;                                                                                                                                                                 \
      if (ctx.address >= MAX_STATE_MEM_SIZE)                                                                                                                             \
         ctx.address = 0;                                                                                                                                                \
   } while (0)

static void __not_in_flash_func(i2c_slave_handler)(i2c_inst_t* i2c, i2c_slave_event_t event) {
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
         dirty = true;
         break;
      default:
         break;
   }
}

void protocol_init() {
   LOG_DEBUG("Init comms I2C...\n");
   i2c_init(I2C_PORT_COMMS, I2C_FREQ_COMMS);
   gpio_set_function(PIN_I2C_SDA_COMMS, GPIO_FUNC_I2C);
   gpio_set_function(PIN_I2C_SCL_COMMS, GPIO_FUNC_I2C);
   gpio_pull_up(PIN_I2C_SDA_COMMS);
   gpio_pull_up(PIN_I2C_SCL_COMMS);

   // clear I2C registers
   memset(mem, 0, MAX_STATE_MEM_SIZE);

   // set readonly info
   set_state16(REG_VERSION_w, SWX_VERSION);
   set_state(REG_CHANNEL_COUNT, CHANNEL_COUNT);
   set_state(REG_CH_CAL_ENABLED, CH_CAL_ENABLED);

   // set current states
   set_state(REG_PSU_ENABLE, is_psu_enabled());

   LOG_DEBUG("Init protocol...\n");
   i2c_slave_init(I2C_PORT_COMMS, I2C_ADDRESS_COMMS, i2c_slave_handler);
}

void protocol_process() {
   if (!dirty)
      return;
   dirty = false;

   // update hardware PSU state
   set_psu_enabled(get_state(REG_PSU_ENABLE));

   // run requested cmd
   const uint8_t state = get_state(REG_CMD);
   if (state) {
      const uint8_t ch_index = (state & REG_CMD_CH_MASK) >> REG_CMD_CH_BIT;
      const uint16_t arg0 = get_state16(REG_CMD + 1);

      set_state(REG_CMD, 0); // clear CMD, so I2C master can set another

      switch ((state & REG_CMD_ACTION_MASK) >> REG_CMD_ACTION_BIT) {
         case CMD_PULSE:
            output_pulse(ch_index, arg0, arg0, time_us_32());
            break;
         case CMD_SET_POWER:
            output_set_power(ch_index, arg0);
            break;
         case CMD_PARAM_UPDATE:
            parameter_update(ch_index, (param_t)arg0);
            break;
         default:
            break;
      }
   }
}