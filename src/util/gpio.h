#ifndef _GPIO_H
#define _GPIO_H

#include <hardware/gpio.h>

static inline void init_gpio(uint pin, bool out, bool value) {
   gpio_init(pin);
   gpio_set_dir(pin, out);
   gpio_put(pin, value);
}

static inline void gpio_toggle(uint pin) {
   gpio_put(pin, !gpio_get(pin));
}

#endif // _GPIO_H