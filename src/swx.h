#ifndef _SWX_H
#define _SWX_H

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include <pico/config.h>

#define SWX_VERSION (1)

#if !defined(PIN_LED) && defined(PICO_DEFAULT_LED_PIN)
#define PIN_LED (PICO_DEFAULT_LED_PIN)
#else
#warning "PIN_LED undefined! LED disabled."
#endif

#ifndef PIN_LED_OK
#warning "PIN_LED_OK undefined! OK LED disabled."
#endif

#ifndef PIN_REG_EN
#warning "PIN_REG_EN undefined! Assuming PSU always on."
#endif

#endif  // _SWX_H