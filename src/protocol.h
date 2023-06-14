#ifndef _PROTOCOL_H
#define _PROTOCOL_H

#include "swx.h"
#include "message.h"

typedef enum {
   MSG_CH_NONE = 0,
   MSG_CH_SPI = 1,
   MSG_CH_UART = 2,
   MSG_CH_STDIO = 4,
} msg_ch_t;

void protocol_process(msg_ch_t src);

void parse_message(msg_ch_t origin, uint8_t ctrl);

bool is_readable(msg_ch_t src);

void write_blocking(msg_ch_t dst, const void* data, uint8_t size);

int read_blocking(msg_ch_t src, uint8_t* data, uint8_t size);

#endif // _PROTOCOL_H