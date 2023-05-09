#ifndef _AUDIO_H
#define _AUDIO_H

#include "swx.h"
#include "channel.h"

void audio_process(channel_t* ch, uint8_t channel, uint16_t power);

#endif // _AUDIO_H