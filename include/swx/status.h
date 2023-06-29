#ifndef _STATUS_H
#define _STATUS_H

#include <inttypes.h>

typedef enum {
   CHANNEL_INVALID = 0,
   CHANNEL_FAULT,
   CHANNEL_UNCALIBRATED,
   CHANNEL_CALIBRATING,
   CHANNEL_READY,
} channel_status_t;

#endif // _STATUS_H