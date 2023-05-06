#ifndef _ANALOG_CAPTURE_H
#define _ANALOG_CAPTURE_H

#include "swx.h"

#define TOTAL_AUDIO_CHANNELS (3)

typedef enum {
   AUDIO_CHANNEL_NONE = 0,
   AUDIO_CHANNEL_MIC,
   AUDIO_CHANNEL_LEFT,
   AUDIO_CHANNEL_RIGHT,
} audio_channel_t;

void analog_capture_init();
void analog_capture_free();

void analog_capture_start();
void analog_capture_stop();

bool fetch_get_audio_buffer(audio_channel_t channel, uint16_t* samples, uint8_t** buffer);

#endif // _ANALOG_CAPTURE_H