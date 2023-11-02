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
#ifndef _STATE_H
#define _STATE_H

#include "swx.h"
#include "message.h"

// Set an 8-bit I2C accessible value at the given address. Bypasses readonly region defined by READ_ONLY_ADDRESS_BOUNDARY.
// Value is accessible by the I2C master.
static inline void set_state(uint16_t address, uint8_t value) {
   extern uint8_t mem[MAX_STATE_MEM_SIZE];
   mem[address] = value;
}

// Set a 16-bit I2C accessible value at the current and next address. Bypasses readonly region defined by READ_ONLY_ADDRESS_BOUNDARY.
// Value is accessible by the I2C master.
// address+0: lower byte
// address+1: upper byte
static inline void set_state16(uint16_t address, uint16_t value) {
   extern uint8_t mem[MAX_STATE_MEM_SIZE];
   mem[address] = value & 0xFF;
   mem[address + 1] = (value >> 8) & 0xFF;
}

// Returns the 8-bit I2C accessible value at the given address. Value is accessible by the I2C master.
static inline uint8_t get_state(uint16_t address) {
   extern uint8_t mem[MAX_STATE_MEM_SIZE];
   return mem[address];
}

// Returns the 16-bit I2C accessible value by using the current and next address. Value is accessible by the I2C master.
static inline uint16_t get_state16(uint16_t address) {
   extern uint8_t mem[MAX_STATE_MEM_SIZE];
   return mem[address] | (mem[address + 1] << 8);
}

#endif // _STATE_H