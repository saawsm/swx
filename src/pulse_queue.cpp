#include "pulse_queue.h"

swx::PulseQueue::PulseQueue(uint8_t count, uint16_t minimumPulseGap) : nextPulseTime(0), isolated(minimumPulseGap > 0), minimumPulseGap(minimumPulseGap) {
   queue_init(&queue, sizeof(Pulse), count);
}

swx::PulseQueue::~PulseQueue() {
   queue_free(&queue);
}

bool swx::PulseQueue::enqueue(uint8_t channel, uint16_t pos_us, uint16_t neg_us) {
   Pulse pulse = {
       .channel = channel,
       .pos_us = pos_us,
       .neg_us = neg_us,
   };
   return queue_try_add(&queue, &pulse);
}

bool swx::PulseQueue::dequeue(Pulse& pulse) {
   if (!queue_try_remove(&queue, &pulse))
      return false;

   if (!isolated) { // If channel isolation is on, we need to delay pulses
      if (time_us_32() < nextPulseTime)
         return false;
      nextPulseTime = time_us_32() + pulse.pos_us + pulse.neg_us + minimumPulseGap;
   }

   return true;
}