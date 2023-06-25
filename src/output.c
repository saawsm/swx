#include "output.h"

#include <pico/util/queue.h>

#include "util/i2c.h"
#include "util/gpio.h"

#include "status.h"
#include "state.h"

#include "pulse_gen.pio.h"
#define CHANNEL_PIO_PROGRAM (pio_pulse_gen_program)

#define DAC_MAX_VALUE (4095) // Assume DAC is 12-bit // TODO: Move to a better location

#if defined(ADC_MEAN) && defined(ADC_MEAN_TRIM_AMOUNT)
#define USE_ADC_MEAN
#endif
#ifdef USE_ADC_MEAN
#if ADC_MEAN <= (ADC_MEAN_TRIM_AMOUNT * 2)
#error Not enough ADC samples for trimmed mean!
#endif
#include <stdlib.h>
#endif

#ifdef CH_IGNORE_CAL_ERRORS
#warning "Channel calibration errors are ignored! Output modules could overload!"
#endif

#ifdef CH_CAL_THRESHOLD_OK
#ifndef CH1_CAL_THRESHOLD_OK
#define CH1_CAL_THRESHOLD_OK (CH_CAL_THRESHOLD_OK)
#endif
#ifndef CH2_CAL_THRESHOLD_OK
#define CH2_CAL_THRESHOLD_OK (CH_CAL_THRESHOLD_OK)
#endif
#ifndef CH3_CAL_THRESHOLD_OK
#define CH3_CAL_THRESHOLD_OK (CH_CAL_THRESHOLD_OK)
#endif
#ifndef CH4_CAL_THRESHOLD_OK
#define CH4_CAL_THRESHOLD_OK (CH_CAL_THRESHOLD_OK)
#endif
#endif

#ifdef CH_CAL_THRESHOLD_OVER
#ifndef CH1_CAL_THRESHOLD_OVER
#define CH1_CAL_THRESHOLD_OVER (CH_CAL_THRESHOLD_OVER)
#endif
#ifndef CH2_CAL_THRESHOLD_OVER
#define CH2_CAL_THRESHOLD_OVER (CH_CAL_THRESHOLD_OVER)
#endif
#ifndef CH3_CAL_THRESHOLD_OVER
#define CH3_CAL_THRESHOLD_OVER (CH_CAL_THRESHOLD_OVER)
#endif
#ifndef CH4_CAL_THRESHOLD_OVER
#define CH4_CAL_THRESHOLD_OVER (CH_CAL_THRESHOLD_OVER)
#endif
#endif

#ifdef CH_CAL_OFFSET
#ifndef CH1_CAL_OFFSET
#define CH1_CAL_OFFSET (CH_CAL_OFFSET)
#endif
#ifndef CH2_CAL_OFFSET
#define CH2_CAL_OFFSET (CH_CAL_OFFSET)
#endif
#ifndef CH3_CAL_OFFSET
#define CH3_CAL_OFFSET (CH_CAL_OFFSET)
#endif
#ifndef CH4_CAL_OFFSET
#define CH4_CAL_OFFSET (CH_CAL_OFFSET)
#endif
#endif

#define CH(pinGateA, pinGateB, dacChannel, adcChannel, pio_hw, sm_index, calThresholdOk, calThresholdOver, calOffset)                                                    \
   {                                                                                                                                                                     \
      .pin_gate_a = (pinGateA), .pin_gate_b = (pinGateB), .dac_channel = (dacChannel), .adc_channel = (adcChannel), .pio = (pio_hw), .sm = (sm_index),                   \
      .cal_threshold_ok = (calThresholdOk), .cal_threshold_over = (calThresholdOver), .cal_offset = (calOffset)                                                          \
   }

// External DAC/ADC interface functions
extern bool set_dac_direct(uint8_t channel, uint16_t value);

extern float adc_compute_volts(uint16_t counts);
extern bool adc_read_counts(uint8_t channel, uint16_t* counts);

typedef struct {
   uint8_t channel;
   uint16_t power;
} pwr_cmd_t;

typedef struct {
   uint32_t abs_time_us;

   uint8_t channel;

   uint16_t pos_us;
   uint16_t neg_us;
} pulse_t;

typedef struct {
   const uint8_t pin_gate_a; // GPIO pin for NFET gate A
   const uint8_t pin_gate_b; // GPIO pin for NFET gate B

   const uint8_t dac_channel;
   const uint8_t adc_channel;

   const PIO pio;                  // Hardware PIO instance
   const int sm;                   // PIO state machine index

   const float cal_threshold_ok;   // OK voltage threshold for calibration
   const float cal_threshold_over; // Over voltage threshold for calibration
   const uint16_t cal_offset;      // Calibration offset
} channel_def_t;

static float read_voltage(const channel_def_t* ch);

static const channel_def_t channels[CHANNEL_COUNT] = {
    CH(PIN_CH1_GA, PIN_CH1_GB, CH1_DAC_CHANNEL, CH1_ADC_CHANNEL, pio0, 0, CH1_CAL_THRESHOLD_OK, CH1_CAL_THRESHOLD_OVER, CH1_CAL_OFFSET),
#if CHANNEL_COUNT > 1
    CH(PIN_CH2_GA, PIN_CH2_GB, CH2_DAC_CHANNEL, CH2_ADC_CHANNEL, pio0, 1, CH2_CAL_THRESHOLD_OK, CH2_CAL_THRESHOLD_OVER, CH2_CAL_OFFSET),
#if CHANNEL_COUNT > 2
    CH(PIN_CH3_GA, PIN_CH3_GB, CH3_DAC_CHANNEL, CH3_ADC_CHANNEL, pio0, 2, CH3_CAL_THRESHOLD_OK, CH3_CAL_THRESHOLD_OVER, CH3_CAL_OFFSET),
#if CHANNEL_COUNT > 3
    CH(PIN_CH4_GA, PIN_CH4_GB, CH4_DAC_CHANNEL, CH4_ADC_CHANNEL, pio0, 3, CH4_CAL_THRESHOLD_OK, CH4_CAL_THRESHOLD_OVER, CH4_CAL_OFFSET),
#endif
#endif
#endif
};

static int pio_offsets[NUM_PIOS] = {-1};        // Lookup Table: PIO index -> PIO program offset
static uint8_t pio_sm_counters[NUM_PIOS] = {0}; // Lookup Table: PIO index -> Number of initialized PIO state machines

static queue_t pulse_queue;
static queue_t power_queue;

static pulse_t pulse;           // current pulse
static bool fetch_pulse = true; // if true, fetch next pulse from pulse_queue

void output_init() {
   LOG_DEBUG("Init output...\n");

// Assume PSU always on if pin is undefined.
#ifdef PIN_REG_EN
   init_gpio(PIN_REG_EN, GPIO_OUT, 0);
#endif
   set_psu_enabled(false);

   queue_init(&pulse_queue, sizeof(pulse_t), 10);
   queue_init(&power_queue, sizeof(pwr_cmd_t), 10);

   // Print I2C devices found on bus
   i2c_scan(I2C_PORT_PERIF); // TODO: Validate missing I2C peripheral devices

   for (uint8_t ch_index = 0; ch_index < CHANNEL_COUNT; ch_index++) {
      const channel_def_t* ch = &channels[ch_index];
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

      set_state(REG_CHn_STATUS + ch_index, CHANNEL_UNCALIBRATED);
   }

   fetch_pulse = true;
}

void output_free() {
   LOG_DEBUG("Free output...\n");

   set_psu_enabled(false);

   queue_free(&pulse_queue);
   queue_free(&power_queue);

   for (uint8_t ch_index = 0; ch_index < CHANNEL_COUNT; ch_index++) {
      const channel_def_t* ch = &channels[ch_index];

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

      set_state(REG_CHn_STATUS + ch_index, CHANNEL_INVALID);
   }
}

bool output_calibrate_all() {
   LOG_INFO("Starting calibration of all enabled channels...\n");

   // Clear power cmd queue
   pwr_cmd_t cmd;
   while (queue_try_remove(&power_queue, &cmd))
      tight_loop_contents();

   // Enable PSU
   const bool powerWasOn = is_psu_enabled();
   set_psu_enabled(true);

   bool success = true;
   for (uint8_t ch_index = 0; ch_index < CHANNEL_COUNT; ch_index++) {
      if (((CH_CAL_ENABLED >> ch_index) & 1) == 0) { // Skip calibration for disabled channel slots
         LOG_WARN("Skipping calibration of channel %u...\n", ch_index);
         continue;
      }

      const uint16_t ch_status = REG_CHn_STATUS + ch_index;
      const uint16_t ch_cal_value = REG_CHnn_CAL_VALUE + (ch_index * 2);

      if (get_state(ch_status) == CHANNEL_INVALID) { // This should not happen...
         success = false;
         LOG_ERROR("Calibration failed! Invalid status - ERROR! Aborting calibration...\n");
         break;
      }

      const channel_def_t* ch = &channels[ch_index];

      LOG_DEBUG("Calibrating channel: pio=%u sm=%d\n", pio_get_index(ch->pio), ch->sm);

      set_state(ch_status, CHANNEL_CALIBRATING);

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

            } else if (voltage > ch->cal_threshold_ok) { // self test ok
               LOG_DEBUG("Calibration: pio=%u sm=%d dac=%d voltage=%.3fv - OK\n", pio_index, ch->sm, dacValue, voltage);
               set_state16(ch_cal_value, dacValue);
               set_state(ch_status, CHANNEL_READY);
               break;
            }

            sleep_ms(5);
         }
      }

      // Switch off power
      set_dac_direct(ch->dac_channel, DAC_MAX_VALUE);

#ifdef CH_IGNORE_CAL_ERRORS
      // If errors, ignore and use max range for calibration values. Warning: Output driver could be overdriven at higher power levels
      if (get_state(ch_status) != CHANNEL_READY) {
         LOG_WARN("Ignoring calibration! Using default calibration! pio=%u sm=%d\n", pio_index, ch->sm);
         set_state16(ch_cal_value, DAC_MAX_VALUE - ch->cal_offset);
         set_state(ch_status, CHANNEL_READY);
      }
#endif

      if (get_state(ch_status) == CHANNEL_READY) {
         // Init PIO state machine with pulse gen program
         // Must be done here, since PIO uses different GPIO muxing compared to regular GPIO
         pulse_gen_program_init(ch->pio, ch->sm, pio_offsets[pio_index], ch->pin_gate_a, ch->pin_gate_b);
         pio_sm_set_enabled(ch->pio, ch->sm, true); // Start the PIO state machine
      } else {
         success = false;
         set_state(ch_status, CHANNEL_FAULT);
         LOG_ERROR("Calibration failed! pio=%u sm=%d - ERROR!\n", pio_index, ch->sm);
      }
   }

   set_psu_enabled(success && powerWasOn);

   if (success) {
      LOG_INFO("Calibration successful!\n");
   } else {
      LOG_WARN("Calibration failed for one or more channels! Disabled power...\n");
   }

   return success;
}

void output_process_pulses() {
   if (fetch_pulse) {
      if (queue_try_remove(&pulse_queue, &pulse)) {
         if (pulse.abs_time_us < time_us_32() + 1000000ul) // ignore pulses with wait times above 1 second
            fetch_pulse = false;
      }
   } else if (pulse.abs_time_us < time_us_32()) {
      static const uint16_t PW_MAX = (1 << PULSE_GEN_BITS) - 1;

      const channel_def_t* ch = &channels[pulse.channel];
      if (get_state(REG_CHn_STATUS + pulse.channel) == CHANNEL_READY && pulse.pos_us <= PW_MAX && pulse.neg_us <= PW_MAX) {
         if (!pio_sm_is_tx_fifo_full(ch->pio, ch->sm)) {
            static_assert(PULSE_GEN_BITS <= 16); // Ensure we can fit the bits
            uint32_t val = (pulse.pos_us << PULSE_GEN_BITS) | (pulse.neg_us);
            pio_sm_put(ch->pio, ch->sm, val);
         } else {
            LOG_WARN("PIO pulse queue full! pio=%u sm=%d\n", pio_get_index(ch->pio), ch->sm);
         }
      }

      fetch_pulse = true;
   }
}

void output_process_power() {
   pwr_cmd_t cmd;
   if (queue_try_remove(&power_queue, &cmd)) {
      const channel_def_t* ch = &channels[cmd.channel];

      if (get_state(REG_CHn_STATUS + cmd.channel) != CHANNEL_READY)
         return;

      if (cmd.power > CHANNEL_POWER_MAX)
         cmd.power = CHANNEL_POWER_MAX;

      const uint16_t cal_value = get_state16(REG_CHnn_CAL_VALUE + (cmd.channel * 2));
      int16_t dacValue = (cal_value + ch->cal_offset) - (cmd.power * 2);

      if (dacValue < 0 || dacValue > DAC_MAX_VALUE) {
         LOG_ERROR("Invalid power calculated! pio=%u sm=%d pwr=%u dac=%d - ERROR!\n", pio_get_index(ch->pio), ch->sm, cmd.power, dacValue);
         return;
      }

      // LOG_FINE("Setting power: pio=%u sm=%d pwr=%u dac=%u\n", pio_get_index(ch->pio), ch->sm, power, dacValue);

      set_dac_direct(ch->dac_channel, (uint16_t)dacValue);
   }
}

bool output_pulse(uint8_t ch_index, uint16_t pos_us, uint16_t neg_us, uint32_t abs_time_us) {
   pulse_t pulse = {
       .channel = ch_index,
       .pos_us = pos_us,
       .neg_us = neg_us,
       .abs_time_us = abs_time_us,
   };
   return queue_try_add(&pulse_queue, &pulse);
}

void output_set_power(uint8_t ch_index, uint16_t power) {
   pwr_cmd_t cmd = {.channel = ch_index, .power = power};
   queue_try_add(&power_queue, &cmd);
}

void set_psu_enabled(bool enabled) {
#ifdef PIN_REG_EN
   const bool oldState = is_psu_enabled();

   gpio_put(PIN_REG_EN, enabled);

   if (oldState != enabled) {
      if (enabled) {
         LOG_INFO("Enabling power...\n");
         sleep_ms(100); // wait for reg to stabilize
      } else {
         LOG_INFO("Disabling power...\n");
      }
   }
#endif
}

bool is_psu_enabled() {
#ifdef PIN_REG_EN
   return gpio_get(PIN_REG_EN);
#else
   return true;
#endif
}

#ifdef USE_ADC_MEAN
static int cmpfunc(const void* a, const void* b) {
   return *(uint16_t*)a - *(uint16_t*)b;
}
#endif

static float read_voltage(const channel_def_t* ch) {
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