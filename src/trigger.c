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
#include "trigger.h"

#include <hardware/gpio.h>

#include "parameter.h"
#include "message.h"
#include "state.h"
#include "pulse_gen.h"

#if TRIGGER_COUNT > 0
static volatile bool triggers_dirty = false;
static uint32_t next_update_time_us = 0;

static bool previous_results[MAX_TRIGS] = {0};

void __not_in_flash_func(trigger_callback)(uint gpio, uint32_t events) {
   triggers_dirty = true;
}
#endif

void triggers_init() {
#if TRIGGER_COUNT > 0
   gpio_set_irq_enabled_with_callback(PIN_TRIGGER1, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &trigger_callback);
#if TRIGGER_COUNT > 1
   gpio_set_irq_enabled_with_callback(PIN_TRIGGER2, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &trigger_callback);
#if TRIGGER_COUNT > 2
   gpio_set_irq_enabled_with_callback(PIN_TRIGGER3, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &trigger_callback);
#if TRIGGER_COUNT > 3
   gpio_set_irq_enabled_with_callback(PIN_TRIGGER4, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &trigger_callback);
#endif
#endif
#endif
#endif
}

void triggers_process() {
#if TRIGGER_COUNT > 0

   // only process triggers when GPIO changes and no faster than 50 ms
   if (!triggers_dirty || time_us_32() > next_update_time_us)
      return;
   next_update_time_us = time_us_32() + 50000ul; // every 50 ms

   // get trigger IO state as bit field (LSB is trigger 1)
   const uint8_t state = (gpio_get(PIN_TRIGGER1) << 0)
#if TRIGGER_COUNT > 1
                         | (gpio_get(PIN_TRIGGER2) << 1)
#if TRIGGER_COUNT > 2
                         | (gpio_get(PIN_TRIGGER3) << 2)
#if TRIGGER_COUNT > 3
                         | (gpio_get(PIN_TRIGGER4) << 3)
#endif
#endif
#endif
       ;

   for (uint32_t trig_index = 0; trig_index < MAX_TRIGS; trig_index++) {
      const uint8_t offset = TRIG_SIZE * trig_index;

      const uint8_t input = get_state(offset + REG_TRIGn_INPUT);
      const uint8_t trigger_mask = input >> 4;
      const uint8_t trigger_invert_mask = input & 0xf;

      if (!trigger_mask)
         continue;

      const uint8_t output = get_state(offset + REG_TRIGn_OUTPUT);
      const trigger_op_t trigger_op = output >> 4;
      const bool output_invert = !!(output & 0xf);

      if (trigger_op == TRIGGER_OP_DDD)
         continue;

      const uint16_t action = get_state16(offset + REG_TRIGn_ACTION_w);
      const uint8_t action_start = action >> 8;
      const uint8_t action_end = action & 0xff;
      if (!action || (action_start == action_end))
         continue;

      const uint8_t trig_state = (state & trigger_mask) ^ trigger_invert_mask;

      bool result;
      switch (trigger_op) {
         case TRIGGER_OP_OOO: // t1 || t2 || t3 || t4
            result = !!trig_state;
            break;
         case TRIGGER_OP_OOA: // t1 || t2 || t3 && t4
            result = !!(trig_state & 0b0011) || ((trig_state & 0b1100) == 0b1100);
            break;
         case TRIGGER_OP_OAO: // t1 || t2 && t3 || t4
            result = !!(trig_state & 0b1001) || ((trig_state & 0b0110) == 0b0110);
            break;
         case TRIGGER_OP_OAA: // t1 || t2 && t3 && t4
            result = !!(trig_state & 0b0001) || ((trig_state & 0b1110) == 0b1110);
            break;
         case TRIGGER_OP_AOO: // t1 && t2 || t3 || t4
            result = !!(trig_state & 0b1100) || ((trig_state & 0b0011) == 0b0011);
            break;
         case TRIGGER_OP_AOA: // t1 && t2 || t3 && t4
            result = ((trig_state & 0b1100) == 0b1100) || ((trig_state & 0b0011) == 0b0011);
            break;
         case TRIGGER_OP_AAO: // t1 && t2 && t3 || t4
            result = !!(trig_state & 0b1000) || ((trig_state & 0b0111) == 0b0111);
            break;
         case TRIGGER_OP_AAA: // t1 && t2 && t3 && t4
            result = (trig_state == 0b1111);
            break;
         default:
            continue;
      }

      result ^= output_invert;

      // only trigger action if result is true and has changed since last time
      if (result != previous_results[trig_index]) {
         previous_results[trig_index] = result;
         if (result)
            execute_action_list(action_start, action_end);
      }
   }

   triggers_dirty = false;
#endif
}