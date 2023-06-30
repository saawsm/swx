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
#ifndef _SWX_H
#define _SWX_H

#include <pico/stdlib.h>

#include <stdio.h>
#include <string.h>

#define SWX_VERSION (1100)

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