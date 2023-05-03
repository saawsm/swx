#ifndef _SWX_H
#define _SWX_H

#include <pico/stdlib.h>
#include <stdio.h>
#include <string.h>

#define SWX_VERSION (1100)

#if defined(RASPBERRYPI_PICO)
#define BOARD_PICO
#elif defined(RASPBERRYPI_PICO_W)
#define BOARD_PICO_W
#endif

#if !defined(PIN_LED) && defined(PICO_DEFAULT_LED_PIN)
#define PIN_LED (PICO_DEFAULT_LED_PIN)
#elif !defined(PIN_LED)
#warning "PIN_LED undefined! LED disabled."
#endif

#ifndef PIN_REG_EN
#warning "PIN_REG_EN undefined! Assuming PSU always on."
#endif

#define LOG_INFO(...) printf(__VA_ARGS__)
#define LOG_DEBUG(...) printf(__VA_ARGS__)
#define LOG_FINE(...) printf(__VA_ARGS__)
#define LOG_FINE_MSG(...) printf(__VA_ARGS__)

#define LOG_WARN(...) printf(__VA_ARGS__)
#define LOG_ERROR(...) printf(__VA_ARGS__)

#define HZ_TO_US(hz) (1000000ul / (hz))

#endif // _SWX_H