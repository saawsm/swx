#include "queued_dac.h"

#include <pico/util/queue.h>

swx::QueuedDac::QueuedDac(Dac* internalDac, uint8_t count) : Dac(internalDac->channelCount, internalDac->maxValue), internalDac(internalDac) {
   queue_init(&queue, sizeof(DacCmd), count);
}

swx::QueuedDac::~QueuedDac() {
   delete internalDac;
   internalDac = nullptr;

   queue_free(&queue);
}

bool swx::QueuedDac::setChannel(uint8_t channel, uint16_t value) {
   DacCmd cmd = {
       .channel = channel,
       .value = value,
   };
   return queue_try_add(&queue, &cmd);
}

bool swx::QueuedDac::process() {
   DacCmd cmd;
   if (queue_try_remove(&queue, &cmd)) {
      internalDac->setChannel(cmd.channel, cmd.value);
      return true;
   }
   return false;
}