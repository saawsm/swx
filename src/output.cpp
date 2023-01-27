#include "output.h"

#include "util.h"
#include "pulse_gen.pio.h"

swx::Output::Output(PIO pio, Adc& adc, Dac& dac) : pio(pio) {
#ifdef PIN_REG_EN
   init_gpio(PIN_REG_EN, GPIO_OUT, 0);
#endif

   // Max of 4 channels per pio program (4 state machines per PIO)
   pio_program_offset = pio_add_program(pio, &pulse_gen_program);

   channels[0] = new Channel(0, PIN_CH1_GA, PIN_CH1_GB, adc, CH1_ADC_ADDRESS, dac, CH1_DAC_ADDRESS, queue, pio, 0, pio_program_offset, CH1_CAL_THRESHOLD_OK,
                             CH1_CAL_THRESHOLD_OVER, CH1_CAL_OFF_POINT);
   channels[1] = new Channel(1, PIN_CH2_GA, PIN_CH2_GB, adc, CH2_ADC_ADDRESS, dac, CH2_DAC_ADDRESS, queue, pio, 1, pio_program_offset, CH2_CAL_THRESHOLD_OK,
                             CH2_CAL_THRESHOLD_OVER, CH2_CAL_OFF_POINT);
   channels[2] = new Channel(2, PIN_CH3_GA, PIN_CH3_GB, adc, CH3_ADC_ADDRESS, dac, CH3_DAC_ADDRESS, queue, pio, 2, pio_program_offset, CH3_CAL_THRESHOLD_OK,
                             CH3_CAL_THRESHOLD_OVER, CH3_CAL_OFF_POINT);
   channels[3] = new Channel(3, PIN_CH4_GA, PIN_CH4_GB, adc, CH4_ADC_ADDRESS, dac, CH4_DAC_ADDRESS, queue, pio, 3, pio_program_offset, CH4_CAL_THRESHOLD_OK,
                             CH4_CAL_THRESHOLD_OVER, CH4_CAL_OFF_POINT);
}

swx::Output::~Output() {
   setPowerEnabled(false);

   for (uint8_t index = 0; index < CHANNEL_COUNT; index++) {
      if (channels[index]) {
         delete channels[index];
         channels[index] = nullptr;
      }
   }

   pio_remove_program(pio, &pulse_gen_program, pio_program_offset);
}

bool swx::Output::calibrateAll() {
   printf("Starting calibration of all enabled channels...\n");
   setPowerEnabled(true);

   bool allReady = true;
   for (uint8_t index = 0; index < CHANNEL_COUNT; index++) {
      if (((CH_ENABLED >> index) & 1) == 0) continue;  // Dont calibrate disabled channel slots.

      if (channels[index]->calibrate() != Channel::READY) {
         allReady = false;
         break;
      }
   }

   if (allReady) {  // calibration finished, turn on LED
      printf("Calibration successful!\n");
      gpio_put(PIN_LED_OK, 1);
      return true;
   } else {  // calibration failed, blink LED
      printf("Calibration failed for one or more channels! Halting...\n");
      setPowerEnabled(false);
      return false;
   }
}

void swx::Output::process() {
   Pulse pulse;
   if (queue.dequeue(&pulse) && pulse.channel < CHANNEL_COUNT) {
      channels[pulse.channel]->immediatePulse(pulse.pos_us, pulse.neg_us);
   }
}

void swx::Output::setPower(uint8_t channel, uint16_t power) {
   channels[channel]->setPower(power);
}

void swx::Output::pulse(uint8_t channel, uint8_t pos_us, uint8_t neg_us) {
   channels[channel]->pulse(pos_us, neg_us);
}

swx::Channel::Status swx::Output::getStatus(uint8_t channel) const {
   return channels[channel]->getStatus();
}

void swx::Output::setPowerEnabled(bool enabled) {
#ifdef PIN_REG_EN
   const bool oldState = isPowerEnabled();

   gpio_put(PIN_REG_EN, enabled);

   if (oldState != enabled) {
      if (enabled) {
         printf("Enabling power...\n");
         sleep_ms(100);  // wait for reg to stabilize
      } else {
         printf("Disabling power...\n");
      }
   }
#endif
}

bool swx::Output::isPowerEnabled() const {
#ifdef PIN_REG_EN
   return gpio_get(PIN_REG_EN);
#else
   return true;
#endif
}
