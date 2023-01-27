#ifndef _PROTOCOL_H
#define _PROTOCOL_H

#include "swx.h"

#include "comms/cli.h"
#include "output.h"
#include "pulse_func_gen.h"

/*
---------------------------------------------------------------------
          1           |          2           |          3           |
---------------------------------------------------------------------
R/W |  CMD | CH/STATE |                 ARG1 |                 ARG2 |
  0 | 0000 |      000 |             00000000 |             00000000 |
*/
#define MSG_MODE (7)  // Read/Write
#define MSG_MODE_MASK (1 << 7)

#define MSG_CHANNEL (0)  // Channel or command state
#define MSG_CHANNEL_MASK (7 << 0)

#define MSG_CMD (3)
#define MSG_CMD_MASK (15 << 3)
#define MSG_CMD_STATUS (0)       // get swx status (version and channel count)
#define MSG_CMD_PSU (1)          // set/get power supply state
#define MSG_CMD_CH_STATUS (2)    // get channel status (see swx::Channel::Status)
#define MSG_CMD_CH_SRC (3)       // set/get channel pattern source - none, basic, audio, ...
#define MSG_CMD_CH_POWER (4)     // set/get the power level of the channel (acts as a modifier for the parameter power level value)

// get/set channel params
// MSB 4-bits are param (e.g. freq, power, pulse width), LSB 4-bits are target (e.g. min, max, value, etc)
#define MSG_CMD_CH_PARAM (5)

#define MSG_CMD_CH_AI_MUX (6)    // audio source mapping for channel (i2s/adc - left/right audio channel) - used by audio channel sources

#define MSG_CMD_CH_LL_PULSE (7)  // pulse channel (low level - bypass channel source)
#define MSG_CMD_CH_LL_POWER (8)  // set channel power level (low level - bypass channel source)

namespace swx {

class Protocol final {
   Output& output;
   PulseFuncGenerator& funcGen;

   uint8_t lastPulsePos;
   uint8_t lastPulseNeg;

  public:
   Protocol(Output& output, PulseFuncGenerator& funcGen) : output(output), funcGen(funcGen), lastPulsePos(50), lastPulseNeg(50) {}

   void parseCommand(Cli& cli, uint8_t ctrl);
};

}  // namespace swx

#endif  // _PROTOCOL_H