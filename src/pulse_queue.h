#ifndef _PULSE_QUEUE_H
#define _PULSE_QUEUE_H

#include "swx.h"

#include <pico/util/queue.h>

namespace swx {

   struct Pulse {
      uint8_t channel;
      uint16_t pos_us;
      uint16_t neg_us;
   };

   class PulseQueue final {
      queue_t queue;

      uint32_t nextPulseTime;

   public:
      bool isolated;
      uint16_t minimumPulseGap;

   public:
      PulseQueue(uint8_t count = 10, uint16_t minimumPulseGap = MINIMUM_PULSE_GAP_US);
      ~PulseQueue();

      PulseQueue(const PulseQueue&) = delete;
      PulseQueue& operator=(const PulseQueue&) = delete;
      PulseQueue(PulseQueue&&) = delete;
      PulseQueue& operator=(PulseQueue&&) = delete;

      bool enqueue(uint8_t channel, uint16_t pos_us, uint16_t neg_us);

      bool dequeue(Pulse* pulse);
   };

} // namespace swx

#endif // _PULSE_QUEUE_H