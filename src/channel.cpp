#include "channel.h"

#include <hardware/gpio.h>
#include <pico/time.h>

#include "util.h"

#include "pulse_gen.pio.h"

swx::Channel::Channel(uint8_t index, uint8_t pinGateA, uint8_t pinGateB, Adc& adc, uint8_t adcChannel, Dac& dac, uint8_t dacChannel, PulseQueue& queue,
                      pio_hw_t* pio, uint8_t sm, uint pio_program_offset, float calThresholdOk, float calThresholdOver, uint16_t calOffPoint)
    : index(index),
      pinGateA(pinGateA),
      pinGateB(pinGateB),
      dacChannel(dacChannel),
      adcChannel(adcChannel),
      calThresholdOk(calThresholdOk),
      calThresholdOver(calThresholdOver),
      calOffPoint(calOffPoint),
      queue(queue),
      adc(adc),
      dac(dac),
      pio(pio),
      sm(sm),
      pio_program_offset(pio_program_offset),
      calValue(dac.maxValue),
      status(UNINITIALIZED) {
   // Setup gate pins
   init_gpio(pinGateA, GPIO_OUT, 0);
   init_gpio(pinGateB, GPIO_OUT, 0);

   // Switch off power
   dac.setChannel(dacChannel, dac.maxValue);

   // Claim PIO state machine for future use
   pio_sm_claim(pio, sm);
}

swx::Channel::~Channel() {
   status = INVALID;

   // Free PIO state machine
   pio_sm_set_enabled(pio, sm, false);
   pio_sm_unclaim(pio, sm);

   // Ensure gate pins are off by reinitializing GPIO, since PIO setup changed GPIO muxing
   init_gpio(pinGateA, GPIO_OUT, 0);
   init_gpio(pinGateB, GPIO_OUT, 0);

   // Switch off power
   dac.setChannel(dacChannel, dac.maxValue);
}

swx::Channel::Status swx::Channel::calibrate() {
   if (status != UNINITIALIZED) return INVALID;

   printf("Starting channel calibration: index=%d sm=%d\n", index, sm);

   status = CALIBRATING;

   // Switch off power
   dac.setChannel(dacChannel, dac.maxValue);
   sleep_us(100);

   // Validate feedback voltage when unpowered
   float voltage = readVoltage();
   if (voltage > 0.03f) {  // 30mV - ~30mA (0.1R with 10AV)
      printf("Precalibration overvoltage! index=%d sm=%d voltage=%.3fv - ERROR!\n", index, sm, voltage);
#ifndef CH_IGNORE_CAL_ERRORS
      status = FAULT;
      return status;
#endif
   } else {
      printf("Precalibration voltage: index=%d sm=%d voltage=%.3fv - OK\n", index, sm, voltage);
   }

   for (uint16_t dacValue = 4000; dacValue > 2000; dacValue -= 10) {
      dac.setChannel(dacChannel, dacValue);  // Set power level
      sleep_us(100);                         // Stabilize

      // Switch on both nfets
      gpio_put(pinGateA, 1);
      gpio_put(pinGateB, 1);

      sleep_us(50);  // Stabilize, then sample feedback voltage

      voltage = readVoltage();

      // Switch off both nfets
      gpio_put(pinGateA, 0);
      gpio_put(pinGateB, 0);

      printf("Calibrating: index=%d sm=%d dac=%d voltage=%.3fv\n", index, sm, dacValue, voltage);

      // Check if the voltage isn't higher than expected. This shouldn't happen...
      if (voltage > calThresholdOver) {
         printf("Calibration overvoltage! index=%d sm=%d dac=%d voltage=%.3fv - ERROR!\n", index, sm, dacValue, voltage);
         break;

      } else if (voltage > calThresholdOk) {
         printf("Calibration: index=%d sm=%d dac=%d voltage=%.3fv - OK\n", index, sm, dacValue, voltage);
         calValue = dacValue;
         status = READY;

         // Init PIO state machine with pulse gen program
         // Must be done here, since PIO uses different GPIO muxing compared to regular GPIO
         pulse_gen_program_init(pio, sm, pio_program_offset, pinGateA, pinGateB);
         break;
      }

      sleep_ms(5);
   }

   // Switch off power
   dac.setChannel(dacChannel, dac.maxValue);

   if (status != READY) {
#ifndef CH_IGNORE_CAL_ERRORS
      status = FAULT;
      printf("Calibration failed! index=%d sm=%d - ERROR!\n", index, sm);
#else
      printf("Ignoring calibration! Using default calibration! index=%d sm=%d\n", index, sm);
      calValue = dac.maxValue - calOffPoint;
      status = READY;

      // Init PIO state machine with pulse gen program
      pulse_gen_program_init(pio, sm, pio_program_offset, pinGateA, pinGateB);
#endif
   }

   return status;
}

void swx::Channel::setPower(uint16_t power) {
   if (status != READY) return;

   if (power > 1000) power = 1000;

   int16_t dacValue = (calValue + calOffPoint) - (power * 2);

   if (dacValue < 0 || dacValue > dac.maxValue) {
      printf("ERROR: Invalid power calculated! index=%d sm=%d pwr=%u dac=%d\n", index, sm, power, dacValue);
      return;
   }

   // printf("Setting power: index=%d sm=%d pwr=%u dac=%u\n", index, sm, power, dacValue);

   dac.setChannel(dacChannel, dacValue);
}

void swx::Channel::pulse(uint8_t pos_us, uint8_t neg_us) {
   if (status != READY) return;
   queue.enqueue(index, pos_us, neg_us);
}

void swx::Channel::immediatePulse(uint8_t pos_us, uint8_t neg_us) {
   if (status != READY) return;

   if (!pio_sm_is_tx_fifo_full(pio, sm)) {
      static_assert(PULSE_GEN_BITS <= 16);
      uint32_t val = (pos_us << PULSE_GEN_BITS) | (neg_us);
      pio_sm_put(pio, sm, val);
   } else {
      printf("PIO pulse queue full! index=%d sm=%d\n", index, sm);
   }
}

static int cmpfunc(const void* a, const void* b) {
   return reinterpret_cast<int32_t>(a) - reinterpret_cast<int32_t>(b);
}

float swx::Channel::readVoltage() {  // return time: ~640 us
   adc.startConversion(adcChannel);

   while (!adc.conversionComplete()) tight_loop_contents();

   int16_t counts = adc.lastConversion();

   return adc.computeVolts(counts);
}

// Test the PFET by starting from the highest DAC setting which should be fully off, and run until 0, which should be fully on.
// Output the feedback voltage for each DAC value in CSV, so it can be imported into a spreadsheet.
// Characterize can only be called before calibration.
void swx::Channel::characterize() {
   if (status != UNINITIALIZED) return;

   printf("Starting channel characterize: index=%d sm=%d dch=%d\n", index, sm, dacChannel);

   printf("dac_value,volts,mA\n");
   for (int16_t dacValue = dac.maxValue; dacValue > 500; dacValue -= 5) {
      dac.setChannel(dacChannel, dacValue);  // Set power level
      sleep_us(100);                         // Stabilize

      // Switch on both nfets
      gpio_put(pinGateA, 1);
      gpio_put(pinGateB, 1);

      sleep_us(50);  // Stabilize, then sample feedback voltage

      float voltage = readVoltage();

      // Switch off both nfets
      gpio_put(pinGateA, 0);
      gpio_put(pinGateB, 0);

      float ma = voltage * 1000.0f;

      printf("%d,%.3f,%f\n", dacValue, voltage, ma);

      sleep_ms(5);

      if (voltage >= 3.2f) break;  // break if voltage is getting near limit - since module feedback has the potential to go above 3.3V
   }

   // Switch off both nfets
   gpio_put(pinGateA, 0);
   gpio_put(pinGateB, 0);

   dac.setChannel(dacChannel, dac.maxValue);

   printf("Done.\n");
}