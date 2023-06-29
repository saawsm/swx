#ifndef _ANALOG_CAPTURE_H
#define _ANALOG_CAPTURE_H

#include "swx.h"

#define TOTAL_ANALOG_CHANNELS (3)

typedef enum {
   AUDIO_CHANNEL_NONE = 0,

   AUDIO_CHANNEL_MIC,
   AUDIO_CHANNEL_LEFT,
   AUDIO_CHANNEL_RIGHT,
} analog_channel_t;

void analog_capture_init();

void analog_capture_start();
void analog_capture_stop();

bool fetch_analog_buffer(analog_channel_t channel, uint16_t* samples, uint16_t** buffer, uint32_t* capture_end_time_us);

uint32_t get_capture_duration_us(analog_channel_t channel);

#endif // _ANALOG_CAPTURE_H