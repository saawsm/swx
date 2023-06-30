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
#ifndef _GPIO_H
#define _GPIO_H

#include "../swx.h"
#include <hardware/gpio.h>

static inline void init_gpio(uint pin, bool out, bool value) {
   gpio_init(pin);
   gpio_set_dir(pin, out);
   gpio_put(pin, value);
}

static inline void gpio_toggle(uint pin) {
   gpio_put(pin, !gpio_get(pin));
}

static inline void gpio_assertt(uint pin, bool active_high) {
   gpio_put(pin, active_high);
   sleep_us(5); // ESP32 200kHz limit
   gpio_put(pin, !active_high);
}

// Assert an active low GPIO pin
static inline void gpio_assert(uint pin) {
   gpio_assertt(pin, false);
}

#endif // _GPIO_H