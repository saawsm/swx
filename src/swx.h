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

// See pico_enable_stdio_usb and pico_enable_stdio_uart in cmake for log targets
#ifndef NDEBUG
#define LOG_DEBUG(...) printf(__VA_ARGS__) // general debugging
#define LOG_FINE(...) printf(__VA_ARGS__)  // inner loops, etc
#else
#define LOG_DEBUG(...)
#define LOG_FINE(...)
#endif

#define LOG_INFO(...) printf(__VA_ARGS__)  // general info

#define LOG_WARN(...) printf(__VA_ARGS__)  // recoverable warnings
#define LOG_ERROR(...) printf(__VA_ARGS__) // errors (usually not recoverable)

#define HZ_TO_US(hz) (1000000ul / (hz))

#endif // _SWX_H