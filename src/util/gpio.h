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