#ifndef _STATE_H
#define _STATE_H

#define MAX_STATE_MEM_SIZE (256)

static inline void set_state(uint8_t address, uint8_t value) {
   extern uint8_t mem[MAX_STATE_MEM_SIZE];
   mem[address] = value;
}

static inline void set_state16(uint8_t address, uint16_t value) {
   extern uint8_t mem[MAX_STATE_MEM_SIZE];
   mem[address] = value & 0xFF;
   mem[address + 1] = (value >> 8) & 0xFF;
}

static inline uint8_t get_state(uint8_t address) {
   extern uint8_t mem[MAX_STATE_MEM_SIZE];
   return mem[address];
}

static inline uint16_t get_state16(uint8_t address) {
   extern uint8_t mem[MAX_STATE_MEM_SIZE];
   return mem[address] | (mem[address + 1] << 8);
}

#endif // _STATE_H