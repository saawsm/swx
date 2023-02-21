#ifndef _PROTOCOL_H
#define _PROTOCOL_H

#include "swx.h"

#include "comms/cli.h"
#include "output.h"
#include "pulse_func_gen.h"

#include "message.h"

namespace swx {

   class Protocol final {
      Output& output;
      PulseFuncGenerator& funcGen;

      uint16_t lastPulse;

   public:
      Protocol(Output& output, PulseFuncGenerator& funcGen) : output(output), funcGen(funcGen), lastPulse(50) {}

      void parseCommand(Cli& cli, uint8_t ctrl);
   };

} // namespace swx

#endif // _PROTOCOL_H