#ifndef _CHANNEL_H
#define _CHANNEL_H

#include "swx.h"

#include <hardware/pio.h>

#define CHANNEL_POWER_MAX (1000)

#define CH(pinGateA, pinGateB, dacChannel, adcChannel, pio_hw, sm_index, calThresholdOk, calThresholdOver, calOffset)                                                    \
   {                                                                                                                                                                     \
      .pin_gate_a = (pinGateA), .pin_gate_b = (pinGateB), .dac_channel = (dacChannel), .adc_channel = (adcChannel), .pio = (pio_hw), .sm = (sm_index),                   \
      .cal_threshold_ok = (calThresholdOk), .cal_threshold_over = (calThresholdOver), .cal_offset = (calOffset)                                                          \
   }

typedef enum {
   CHANNEL_INVALID = 0,
   CHANNEL_FAULT,
   CHANNEL_UNCALIBRATED,
   CHANNEL_CALIBRATING,
   CHANNEL_READY,
} channel_status_t;

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

   uint16_t cal_value;             // Calibration value

   channel_status_t status;
} channel_t;

void channel_init(channel_t* ch);

void channel_free(channel_t* ch);

channel_status_t channel_calibrate(channel_t* ch);

void channel_pulse(channel_t* ch, uint16_t pos_us, uint16_t neg_us);

void channel_set_power(channel_t* ch, uint16_t power);

#endif // _CHANNEL_H