#include "protocol.h"

void swx::Protocol::parseCommand(Cli& cli, uint8_t ctrl) {
   bool isWrite = (ctrl & MSG_MODE_MASK) >> MSG_MODE;
   uint8_t channel = (ctrl & MSG_CHANNEL_MASK) >> MSG_CHANNEL;

   if (channel >= CHANNEL_COUNT) return;

   switch ((ctrl & MSG_CMD_MASK) >> MSG_CMD) {
      case MSG_CMD_STATUS: {  // respond with swx status information
         uint8_t buffer[4] = {RES_START, 2, SWX_VERSION, CHANNEL_COUNT};
         cli.writeBlocking(buffer, 4);
         break;
      }
      case MSG_CMD_PSU: {  // get/set power supply enabled state
         if (isWrite) {
            output.setPowerEnabled(channel > 0);
         } else {
            cli.writeResponse(output.isPowerEnabled());
         }
         break;
      }
      case MSG_CMD_CH_STATUS: {  // respond with channel status
         uint8_t status = (uint8_t)output.getStatus(channel);
         cli.writeResponse(status);
         break;
      }
      case MSG_CMD_CH_LL_PULSE: {  // pulse channel with last used values if read mode
         if (isWrite) {
            uint8_t buffer[2];
            cli.readBlocking(buffer, 2);

            lastPulse = (buffer[0] << 8 | buffer[1]);
         }
         output.pulse(channel, lastPulse);
         break;
      }
      case MSG_CMD_CH_LL_POWER: {  // set low level channel power
         uint8_t buffer[2];
         cli.readBlocking(buffer, 2);

         uint16_t val = (buffer[0] << 8 | buffer[1]);
         output.setPower(channel, val);
         break;
      }
      case MSG_CMD_CH_SRC: {  // get/set channel pulse generation source (e.g. audio generation vs function gen)
         if (isWrite) {
            uint8_t buffer[1];
            cli.readBlocking(buffer, 1);
            funcGen.setSource(channel, static_cast<PulseFuncGenerator::PulseSrc>(buffer[0]));
         } else {
            cli.writeResponse(funcGen.getSource(channel));
         }
         break;
      }
      case MSG_CMD_CH_PARAM: {  // set/get channel parameter - e.g. frequency, pulse width, etc
         if (isWrite) {
            uint8_t buffer[3];
            cli.readBlocking(buffer, 3);

            uint8_t param = buffer[0] >> 4;
            uint8_t target = buffer[0] & 0xf;

            uint16_t val = (buffer[1] << 8 | buffer[2]);

            funcGen.setParameter(channel, static_cast<PulseFuncGenerator::Param>(param), static_cast<PulseFuncGenerator::Target>(target), val);
         } else {
            uint8_t buffer[1];
            cli.readBlocking(buffer, 1);

            uint8_t param = buffer[0] >> 4;
            uint8_t target = buffer[0] & 0xf;

            uint16_t ret = funcGen.getParameter(channel, static_cast<PulseFuncGenerator::Param>(param), static_cast<PulseFuncGenerator::Target>(target));

            uint8_t buf[4] = {RES_START, 2, static_cast<uint8_t>(ret >> 8), static_cast<uint8_t>(ret & 0xff)};
            cli.writeBlocking(buf, 4);
         }
         break;
      }
      case MSG_CMD_CH_POWER: {  // set/get channel power level
         if (isWrite) {
            uint8_t buffer[2];
            cli.readBlocking(buffer, 2);

            uint16_t val = (buffer[0] << 8 | buffer[1]);

            funcGen.setPower(channel, val);
         } else {
            uint16_t ret = funcGen.getPower(channel);

            uint8_t buf[4] = {RES_START, 2, static_cast<uint8_t>(ret >> 8), static_cast<uint8_t>(ret & 0xff)};
            cli.writeBlocking(buf, 4);
         }
         break;
      }
      case MSG_CMD_CH_AI_MUX: {
         break;
      }
      default:
         break;
   }
}