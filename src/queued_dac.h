#ifndef _QUEUED_DAC_H
#define _QUEUED_DAC_H

#include "swx.h"

#include "hardware/dac.h"

#include <pico/util/queue.h>

namespace swx {

   class QueuedDac final : public Dac {
      Dac* internalDac;
      queue_t queue;

      struct DacCmd {
         uint8_t channel;
         uint16_t value;
      };

   public:
      // QueuedDac takes ownership of the provided Dac pointer
      QueuedDac(Dac* internalDac, uint8_t count = 10);
      virtual ~QueuedDac();

      QueuedDac(const QueuedDac&) = delete;
      QueuedDac& operator=(const QueuedDac&) = delete;
      QueuedDac(QueuedDac&&) = delete;
      QueuedDac& operator=(QueuedDac&&) = delete;

      bool process();

      virtual bool setChannel(uint8_t channel, uint16_t value) override;
   };

} // namespace swx

#endif // _QUEUED_DAC_H