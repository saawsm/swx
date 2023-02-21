#ifndef _OUTPUT_H
#define _OUTPUT_H

#include "swx.h"
#include "channel.h"

#include "queued_dac.h"
#include "pulse_queue.h"

#include <hardware/pio.h>

#define CHANNEL_COUNT (4)

#ifndef CH_ENABLED
   #define CH_ENABLED (0b1111)
#endif

namespace swx {

   class Output final {
      const PIO pio;
      uint pio_program_offset;

      PulseQueue queue;

      Channel* channels[CHANNEL_COUNT];

      Output(PIO pio, Adc& adc, Dac& dac);
      ~Output();

      static Output& getInstance(PIO* pio = nullptr, Adc* adc = nullptr, Dac* dac = nullptr) {
         static Output output{*pio, *adc, *dac};
         return output;
      }

   public:
      Output(const Output&) = delete;
      Output& operator=(const Output&) = delete;
      Output(Output&&) = delete;
      Output& operator=(Output&&) = delete;

      bool calibrateAll();
      void process();

      void setPower(uint8_t channel, uint16_t power);

      void pulse(uint8_t channel, uint16_t pos_us, uint16_t neg_us);
      void pulse(uint8_t channel, uint16_t us) { pulse(channel, us, us); }

      swx::Channel::Status getStatus(uint8_t channel) const;

      void setPowerEnabled(bool enabled = true);
      bool isPowerEnabled() const;

      static void init(PIO pio, Adc& adc, Dac& dac) { getInstance(&pio, &adc, &dac); }

      // Must call init() before instance()
      static Output& instance() { return getInstance(); }
   };

} // namespace swx

#endif // _OUTPUT_H