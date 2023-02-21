#ifndef _MESSAGE_H
#define _MESSAGE_H

/*
Messages are variable length with a minimum of one byte. Some messages use the channel index (MSG_CHANNEL_MASK) to represent a boolean state (e.g.
MSG_CMD_PSU) allowing for a single byte message. Most other messages have 2 additional bytes representing a uint16_t, with some messages having an additional 3
bytes.
---------------------------------------------------------------------
          1           |          2           |          3           |
---------------------------------------------------------------------
R/W |  CMD | CH/STATE |                 ARG1 |                 ARG2 |
  0 | 0000 |      000 |             00000000 |             00000000 |
*/

#define MSG_MODE (7) // read/write
#define MSG_MODE_MASK (1 << 7)
#define MSG_MODE_READ (0)
#define MSG_MODE_WRITE (1)

#define MSG_CHANNEL (0) // channel index or command state
#define MSG_CHANNEL_MASK (7 << 0)

#define MSG_CMD (3)
#define MSG_CMD_MASK (15 << 3)
#define MSG_CMD_STATUS (0)    // swx status (version and channel count) (R)
#define MSG_CMD_PSU (1)       // power supply state (R/W)
#define MSG_CMD_CH_STATUS (2) // channel status (R) (see swx::Channel::Status)
#define MSG_CMD_CH_SRC (3)    // channel pulse source (R/W+1)- (see swx::PulseFuncGenerator::PulseSrc) e.g. none, function gen, audio, etc
#define MSG_CMD_CH_POWER (4)  // channel power level percent (R/W+2) - used to scale parameter power level

// channel parameter (R+1/W+3) (see swx::PulseFuncGenerator::Param and swx::PulseFuncGenerator::Target)
// MSB 4-bits are for Parameter, LSB 4-bits are for Target
#define MSG_CMD_CH_PARAM (5)

// Not Implemented
// audio source mapping for channel (i2s/adc - left/right audio channel) - used by audio channel sources
#define MSG_CMD_CH_AI_MUX (6)

// pulse channel (R/W+2) (low level - bypass channel source) - use write mode to remember pulse width, read mode to use last set pulse width value
#define MSG_CMD_CH_LL_PULSE (7)
#define MSG_CMD_CH_LL_POWER (8) // set channel power level (W) +2 (low level - bypass channel source)

#endif // _MESSAGE_H