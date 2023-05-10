#include "swx.h"

#include "channel.h"
#include "util/gpio.h"

#include "pulse_gen.pio.h"

#define CHANNEL_PIO_PROGRAM (pio_pulse_gen_program)

#define DAC_MAX_VALUE (4095) // Assume DAC is 12-bit // TODO: Move to a better location

#ifdef USE_ADC_MEAN
#ifndef ADC_MEAN
#define ADC_MEAN (10)
#endif
#ifndef ADC_MEAN_TRIM_AMOUNT
#define ADC_MEAN_TRIM_AMOUNT (2)
#endif

#if ADC_MEAN <= (ADC_MEAN_TRIM_AMOUNT * 2)
#error Not enough ADC samples for trimmed mean!
#endif
#include <stdlib.h>
#endif

#ifdef CH_IGNORE_CAL_ERRORS
#warning "Channel calibration errors are ignored! Output modules could overload!"
#endif

// External DAC/ADC interface functions

extern bool set_dac_direct(uint8_t channel, uint16_t value);

extern float adc_compute_volts(uint16_t counts);
extern bool adc_read_counts(uint8_t channel, uint16_t* counts);

static float read_voltage(channel_t* ch);

static int pio_offsets[NUM_PIOS] = {-1};        // Lookup Table: PIO index -> PIO program offset
static uint8_t pio_sm_counters[NUM_PIOS] = {0}; // Lookup Table: PIO index -> Number of initialized PIO state machines

static inline void set_param(channel_t* ch, param_t param, target_t target, uint16_t value) {
   parameter_set(&ch->parameters[param], target, value);
}

void channel_init(channel_t* ch) {
   LOG_DEBUG("Init channel: pio=%u sm=%d\n", pio_get_index(ch->pio), ch->sm);

   // Setup gate pins
   init_gpio(ch->pin_gate_a, GPIO_OUT, 0);
   init_gpio(ch->pin_gate_b, GPIO_OUT, 0);

   // Switch off power
   set_dac_direct(ch->dac_channel, DAC_MAX_VALUE);

   // Load PIO program if not already loaded
   const uint pio_index = pio_get_index(ch->pio);
   if (pio_offsets[pio_index] < 0) {
      if (!pio_can_add_program(ch->pio, &CHANNEL_PIO_PROGRAM))
         panic("PIO program cant be added! No program space!");

      LOG_DEBUG("Loading pulse gen PIO program: pio=%d\n", pio_index);
      pio_offsets[pio_index] = pio_add_program(ch->pio, &CHANNEL_PIO_PROGRAM);
   }

   // Claim PIO state machine
   pio_sm_claim(ch->pio, ch->sm);
   pio_sm_counters[pio_index]++; // Increment counter so we know PIO program is in use

   ch->status = CHANNEL_UNCALIBRATED;
   ch->gen_enabled = false;
   ch->power_level = 0;
   ch->state_index = 0;

   // Set default parameter values
   set_param(ch, PARAM_POWER, TARGET_VALUE, 1000);      // 100%

   set_param(ch, PARAM_FREQUENCY, TARGET_MAX, 5000);    // max. 500 Hz (soft limit - only auto cycling)
   set_param(ch, PARAM_FREQUENCY, TARGET_VALUE, 1800);  // 180 Hz

   set_param(ch, PARAM_PULSE_WIDTH, TARGET_MAX, 500);   // max. 500 us
   set_param(ch, PARAM_PULSE_WIDTH, TARGET_VALUE, 150); // 150 us

   set_param(ch, PARAM_ON_TIME, TARGET_MAX, 10000);
   set_param(ch, PARAM_ON_RAMP_TIME, TARGET_MAX, 10000);
   set_param(ch, PARAM_OFF_TIME, TARGET_MAX, 10000);
   set_param(ch, PARAM_OFF_RAMP_TIME, TARGET_MAX, 10000);
}

void channel_free(channel_t* ch) {
   const uint pio_index = pio_get_index(ch->pio);
   LOG_DEBUG("Freeing channel: pio=%u sm=%d\n", pio_index, ch->sm);

   // Disable and unclaim PIO state machine
   if (pio_sm_is_claimed(ch->pio, ch->sm)) {
      LOG_DEBUG("Freeing PIO state machine...\n");
      pio_sm_set_enabled(ch->pio, ch->sm, false);
      pio_sm_unclaim(ch->pio, ch->sm);
      pio_sm_counters[pio_index]--;
   }

   // Remove PIO program if no more state machines are in use
   if (pio_offsets[pio_index] >= 0 && pio_sm_counters[pio_index] == 0) {
      LOG_DEBUG("Removing pulse gen PIO program: pio=%d\n", pio_index);
      pio_remove_program(ch->pio, &CHANNEL_PIO_PROGRAM, pio_offsets[pio_index]);
      pio_offsets[pio_index] = -1;
   }

   // Ensure gate pins are off by reinitializing GPIO, since PIO setup changed GPIO muxing
   init_gpio(ch->pin_gate_a, GPIO_OUT, 0);
   init_gpio(ch->pin_gate_b, GPIO_OUT, 0);

   // Switch off power
   set_dac_direct(ch->dac_channel, DAC_MAX_VALUE);

   ch->status = CHANNEL_INVALID;
   ch->gen_enabled = false;
}

channel_status_t channel_calibrate(channel_t* ch) {
   if (ch->status == CHANNEL_INVALID)
      return CHANNEL_INVALID;

   LOG_DEBUG("Calibrating channel: pio=%u sm=%d\n", pio_get_index(ch->pio), ch->sm);

   ch->status = CHANNEL_CALIBRATING;

   // Disable PIO state machine while calibrating
   pio_sm_set_enabled(ch->pio, ch->sm, false);

   // Ensure gate pins are setup for GPIO and not PIO
   init_gpio(ch->pin_gate_a, GPIO_OUT, 0);
   init_gpio(ch->pin_gate_b, GPIO_OUT, 0);

   const uint pio_index = pio_get_index(ch->pio);

   // Validate feedback voltage when unpowered
   float voltage = read_voltage(ch);
   if (voltage > 0.03f) { // 30mV
      LOG_ERROR("Precalibration overvoltage! pio=%u sm=%d voltage=%.3fv - ERROR!\n", pio_index, ch->sm, voltage);
   } else {
      LOG_DEBUG("Precalibration voltage: pio=%u sm=%d voltage=%.3fv - OK\n", pio_index, ch->sm, voltage);

      for (uint16_t dacValue = 4000; dacValue > 2000; dacValue -= 10) {
         set_dac_direct(ch->dac_channel, dacValue);
         sleep_us(100); // Stabilize

         // Switch on both nfets
         gpio_put(ch->pin_gate_a, 1);
         gpio_put(ch->pin_gate_b, 1);

         sleep_us(50); // Stabilize, then sample feedback voltage

         voltage = read_voltage(ch);

         // Switch off both nfets
         gpio_put(ch->pin_gate_a, 0);
         gpio_put(ch->pin_gate_b, 0);

         LOG_FINE("Calibrating: pio=%u sm=%d dac=%d voltage=%.3fv\n", pio_index, ch->sm, dacValue, voltage);

         // Check if the voltage isn't higher than expected
         if (voltage > ch->cal_threshold_over) {
            LOG_ERROR("Calibration overvoltage! pio=%u sm=%d dac=%d voltage=%.3fv - ERROR!\n", pio_index, ch->sm, dacValue, voltage);
            break;

         } else if (voltage > ch->cal_threshold_ok) {
            LOG_DEBUG("Calibration: pio=%u sm=%d dac=%d voltage=%.3fv - OK\n", pio_index, ch->sm, dacValue, voltage);
            ch->cal_value = dacValue;
            ch->status = CHANNEL_READY;
            break;
         }

         sleep_ms(5);
      }
   }

   // Switch off power
   set_dac_direct(ch->dac_channel, DAC_MAX_VALUE);

#ifdef CH_IGNORE_CAL_ERRORS
   // If errors, ignore and use max range for calibration values. Warning: Output driver could be overdriven at higher power levels
   if (ch->status != CHANNEL_READY) {
      LOG_WARN("Ignoring calibration! Using default calibration! pio=%u sm=%d\n", pio_index, ch->sm);
      ch->cal_value = DAC_MAX_VALUE - ch->cal_offset;
      ch->status = CHANNEL_READY;
   }
#endif

   if (ch->status == CHANNEL_READY) {
      // Init PIO state machine with pulse gen program
      // Must be done here, since PIO uses different GPIO muxing compared to regular GPIO
      pulse_gen_program_init(ch->pio, ch->sm, pio_offsets[pio_index], ch->pin_gate_a, ch->pin_gate_b);
      pio_sm_set_enabled(ch->pio, ch->sm, true); // Start the PIO state machine
   } else {
      ch->status = CHANNEL_FAULT;
      LOG_ERROR("Calibration failed! pio=%u sm=%d - ERROR!\n", pio_index, ch->sm);
   }

   return ch->status;
}

void channel_pulse(channel_t* ch, uint16_t pos_us, uint16_t neg_us) {
   static const uint16_t PW_MAX = (1 << PULSE_GEN_BITS) - 1;

   if (ch->status != CHANNEL_READY || pos_us > PW_MAX || neg_us > PW_MAX)
      return;

   if (!pio_sm_is_tx_fifo_full(ch->pio, ch->sm)) {
      static_assert(PULSE_GEN_BITS <= 16); // Ensure we can fit the bits
      uint32_t val = (pos_us << PULSE_GEN_BITS) | (neg_us);
      pio_sm_put(ch->pio, ch->sm, val);
   } else {
      LOG_WARN("PIO pulse queue full! pio=%u sm=%d\n", pio_get_index(ch->pio), ch->sm);
   }
}

void channel_set_power(channel_t* ch, uint16_t power) {
   if (ch->status != CHANNEL_READY)
      return;

   if (power > CHANNEL_POWER_MAX)
      power = CHANNEL_POWER_MAX;

   int16_t dacValue = (ch->cal_value + ch->cal_offset) - (power * 2);

   if (dacValue < 0 || dacValue > DAC_MAX_VALUE) {
      LOG_ERROR("Invalid power calculated! pio=%u sm=%d pwr=%u dac=%d - ERROR!\n", pio_get_index(ch->pio), ch->sm, power, dacValue);
      return;
   }

   // LOG_FINE("Setting power: pio=%u sm=%d pwr=%u dac=%u\n", pio_get_index(ch->pio), ch->sm, power, dacValue);

   set_dac_direct(ch->dac_channel, (uint16_t)dacValue);
}

#ifdef USE_ADC_MEAN
static int cmpfunc(const void* a, const void* b) {
   return *(uint16_t*)a - *(uint16_t*)b;
}
#endif

static float read_voltage(channel_t* ch) {
#ifdef USE_ADC_MEAN
   uint16_t readings[ADC_MEAN];

   // Read n samples
   for (uint8_t index = 0; index < ADC_MEAN; index++)
      adc_read_counts(ch->adc_channel, &readings[index]);

   // Ignore n highest and lowest values. Average the rest
   uint32_t total = 0;
   qsort(readings, ADC_MEAN, sizeof(uint16_t), cmpfunc);
   for (uint8_t index = ADC_MEAN_TRIM_AMOUNT; index < (ADC_MEAN - ADC_MEAN_TRIM_AMOUNT); index++)
      total += readings[index];

   uint16_t counts = total / (ADC_MEAN - (ADC_MEAN_TRIM_AMOUNT * 2));
#else
   uint16_t counts;
   adc_read_counts(ch->adc_channel, &counts);
#endif
   return adc_compute_volts(counts);
}