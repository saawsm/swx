#include "analog_capture.h"

#include <pico/stdlib.h>

#include <hardware/dma.h>
#include <hardware/irq.h>

#include <hardware/adc.h>
#include <hardware/pio.h>
#include <hardware/clocks.h>
#include "i2s.pio.h"

#define PIN_ADC_BASE (26)

// The number of analog samples per second per channel. Since 3 ADC channels are being sampled the actual sample rate is 3 times bigger.
#define ADC_SAMPLES_PER_SECOND (44100)
#define I2S_SAMPLES_PER_SECOND (44100)

// Number of ADC channels sampled
#define ADC_SAMPLED_CHANNELS (3)
#define I2S_SAMPLED_CHANNELS (2)

#define ADC_CAPTURE_COUNT (1024)                                    // Total samples captured per DMA buffer
#define ADC_SAMPLE_COUNT (ADC_CAPTURE_COUNT / ADC_SAMPLED_CHANNELS) // Number of samples per ADC channel

#define I2S_CAPTURE_COUNT (1024)                                    // Total samples captured per DMA buffer
#define I2S_SAMPLE_COUNT (I2S_CAPTURE_COUNT)                        // Number of samples per I2S channel

static void init_pingpong_dma(const uint channel1, const uint channel2, uint dreq, const volatile void* read_addr, volatile void* write_addr1, volatile void* write_addr2,
                              uint transfer_count, enum dma_channel_transfer_size size, uint irq_num, irq_handler_t handler);
static void dma_channels_abort(uint ch1, uint ch2, uint irq_num);
static void dma_adc_handler();
static void dma_i2s_handler();

// ------------------------------------------------------------------
// ADC Capture Variables
// ------------------------------------------------------------------

static uint dma_adc_ch1;
static uint dma_adc_ch2;

// DMA Ping-Pong Buffers - uint16 since ADC is only 12 bit (~9 ENOB)
static uint16_t adc_capture_buf[2][ADC_CAPTURE_COUNT] __attribute__((aligned(ADC_CAPTURE_COUNT * sizeof(uint16_t))));

volatile uint8_t buf_adc_ready;
volatile uint32_t buf_adc_done_time_us;

// ADC consumer buffers, updated on request
static uint16_t adc_buffers[ADC_SAMPLED_CHANNELS][ADC_SAMPLE_COUNT];

static uint32_t adc_end_time_us; // Cached version of: buf_adc_done_time_us

static const uint32_t adc_capture_duration_us = ADC_CAPTURE_COUNT * (1000000ul / (ADC_SAMPLES_PER_SECOND));

// Lookup Table: Analog channel -> ADC round robin stripe offset
static const uint8_t adc_stripe_offsets[] = {
    [AUDIO_CHANNEL_MIC] = (PIN_AUDIO_MIC - PIN_ADC_BASE),
    [AUDIO_CHANNEL_LEFT] = (PIN_AUDIO_LEFT - PIN_ADC_BASE),
    [AUDIO_CHANNEL_RIGHT] = (PIN_AUDIO_RIGHT - PIN_ADC_BASE),
};

// ------------------------------------------------------------------
// I2S Capture Variables
// ------------------------------------------------------------------

static uint dma_i2s_ch1;
static uint dma_i2s_ch2;

// DMA Ping-Pong Buffers - uint32 since PIO FIFO is 32 bit
static uint32_t i2s_capture_buf[2][I2S_CAPTURE_COUNT] __attribute__((aligned(I2S_CAPTURE_COUNT * sizeof(uint32_t))));

volatile uint8_t buf_i2s_ready;
volatile uint32_t buf_i2s_done_time_us;

// ADC consumer buffers, updated on request
static uint16_t i2s_buffers[I2S_SAMPLED_CHANNELS][I2S_SAMPLE_COUNT];

static uint32_t i2s_end_time_us; // Cached version of: buf_i2s_done_time_us

static const uint32_t i2s_capture_duration_us = I2S_CAPTURE_COUNT * (1000000ul / (I2S_SAMPLES_PER_SECOND));

// Lookup Table: Analog channel -> capture_buf
static uint16_t* buffers[] = {
    [AUDIO_CHANNEL_MIC] = adc_buffers[0],       //
    [AUDIO_CHANNEL_LEFT] = adc_buffers[1],      //
    [AUDIO_CHANNEL_RIGHT] = adc_buffers[2],     //
    [AUDIO_CHANNEL_I2S_LEFT] = i2s_buffers[0],  //
    [AUDIO_CHANNEL_I2S_RIGHT] = i2s_buffers[1], //
};

void analog_capture_init() {
   LOG_DEBUG("Init analog capture...\n");

   adc_gpio_init(PIN_AUDIO_LEFT);
   adc_gpio_init(PIN_AUDIO_RIGHT);
   adc_gpio_init(PIN_AUDIO_MIC);

   adc_init();
   adc_select_input(0);
   adc_set_round_robin((1 << (PIN_AUDIO_LEFT - PIN_ADC_BASE)) | (1 << (PIN_AUDIO_RIGHT - PIN_ADC_BASE)) | (1 << (PIN_AUDIO_MIC - PIN_ADC_BASE)));

   adc_fifo_setup(true,  // Write each completed conversion to the sample FIFO
                  true,  // Enable DMA data request (DREQ)
                  1,     // DREQ (and IRQ) asserted when at least 1 sample present
                  false, // Don't collect error bit
                  false  // Don't reduce samples
   );

   static const uint32_t div = 48000000ul / (ADC_SAMPLES_PER_SECOND * ADC_SAMPLED_CHANNELS);
   adc_set_clkdiv(div - 1);

   // Setup ping-pong DMA for ADC FIFO writing to adc_capture_buf, wrapping once filled
   dma_adc_ch1 = dma_claim_unused_channel(true);
   dma_adc_ch2 = dma_claim_unused_channel(true);
   init_pingpong_dma(dma_adc_ch1, dma_adc_ch2, DREQ_ADC, &adc_hw->fifo, adc_capture_buf[0], adc_capture_buf[1], ADC_CAPTURE_COUNT, DMA_SIZE_16, DMA_IRQ_0,
                     dma_adc_handler);

   // Start channel 1
   buf_adc_ready = 0;
   dma_channel_start(dma_adc_ch1);

   const PIO pio = pio1;
   const uint sm = 3;

   const uint bits_per_sample = 16;

   const uint program_offset = pio_add_program(pio, &pio_i2s_in_program);

   pio_sm_claim(pio, sm);
   pio_i2s_in_program_init(pio, sm, program_offset, PIN_I2S_SD, PIN_I2S_BCLK, bits_per_sample, false);

   const float bitClk = I2S_SAMPLES_PER_SECOND * bits_per_sample * 2.0 /* channels */ * 2.0 /* edges per clock */;
   pio_sm_set_clkdiv(pio, sm, (float)clock_get_hz(clk_sys) / bitClk);

   // Setup ping-pong DMA for I2S PIO FIFO writing to i2s_capture_buf, wrapping once filled
   dma_i2s_ch1 = dma_claim_unused_channel(true);
   dma_i2s_ch2 = dma_claim_unused_channel(true);
   init_pingpong_dma(dma_i2s_ch1, dma_i2s_ch2, pio_get_dreq(pio, sm, false), (volatile void*)&pio->rxf[sm], i2s_capture_buf[0], i2s_capture_buf[1], I2S_CAPTURE_COUNT,
                     DMA_SIZE_32, DMA_IRQ_1, dma_i2s_handler);

   pio_sm_set_enabled(pio, sm, true);

   // Start channel 1
   buf_i2s_ready = 0;
   dma_channel_start(dma_i2s_ch1);
}

void analog_capture_free() {
   LOG_DEBUG("Free analog capture...\n");

   analog_capture_stop();

   dma_channels_abort(dma_adc_ch1, dma_adc_ch2, DMA_IRQ_0);
   dma_channels_abort(dma_i2s_ch1, dma_i2s_ch2, DMA_IRQ_1);

   pio_sm_set_enabled(pio1, 3, false);

   adc_set_round_robin(0);
   adc_fifo_drain();
}

void analog_capture_start() {
   LOG_INFO("Starting analog capture...\n");
   adc_run(true);
}

void analog_capture_stop() {
   LOG_INFO("Stopping analog capture...\n");
   adc_run(false);
}

static void __not_in_flash_func(dma_adc_handler)() {
   if (dma_channel_get_irq0_status(dma_adc_ch1)) {
      adc_select_input(0);

      buf_adc_ready = 0xFE; // lookup index: 0
      buf_adc_done_time_us = time_us_32();

      dma_channel_acknowledge_irq0(dma_adc_ch1);
   } else if (dma_channel_get_irq0_status(dma_adc_ch2)) {
      adc_select_input(0);

      buf_adc_ready = 0xFF; // lookup index: 1
      buf_adc_done_time_us = time_us_32();

      dma_channel_acknowledge_irq0(dma_adc_ch2);
   }
}

static void __not_in_flash_func(dma_i2s_handler)() {
   if (dma_channel_get_irq1_status(dma_i2s_ch1)) {

      buf_i2s_ready = 0xFE; // lookup index: 0
      buf_i2s_done_time_us = time_us_32();

      dma_channel_acknowledge_irq1(dma_i2s_ch1);
   } else if (dma_channel_get_irq1_status(dma_i2s_ch2)) {

      buf_i2s_ready = 0xFF; // lookup index: 1
      buf_i2s_done_time_us = time_us_32();

      dma_channel_acknowledge_irq1(dma_i2s_ch2);
   }
}

bool fetch_analog_buffer(analog_channel_t channel, uint16_t* samples, uint16_t** buffer, uint32_t* capture_end_time_us) {
   switch (channel) {
      case AUDIO_CHANNEL_LEFT:
      case AUDIO_CHANNEL_RIGHT:
      case AUDIO_CHANNEL_MIC: {
         const uint8_t ready = buf_adc_ready; // Local copy, since volatile

         // Check if this channel has new or unprocessed buffer data available
         const bool available = ready & (1 << channel);
         if (available) {
            const uint16_t* src = adc_capture_buf[ready & 1];   // Source DMA buffer
            const uint8_t offset = adc_stripe_offsets[channel]; // ADC round robin offset

            // Unravel interleaved capture buffer, and shift from 12 bit samples to 10 bit
            for (uint x = 0; x < ADC_SAMPLE_COUNT; x++)
               buffers[channel][x] = (src[(x * ADC_SAMPLED_CHANNELS) + offset] & 0xFFF) >> 2;

            adc_end_time_us = buf_adc_done_time_us; // Cache done time

            buf_adc_ready &= ~(1 << channel);       // Clear flag
         }

         *capture_end_time_us = adc_end_time_us;
         *buffer = buffers[channel];
         *samples = ADC_SAMPLE_COUNT;
         return available;
      }
      case AUDIO_CHANNEL_I2S_LEFT:
      case AUDIO_CHANNEL_I2S_RIGHT: {
         const uint8_t ready = buf_i2s_ready; // Local copy, since volatile

         // Check if this channel has new or unprocessed buffer data available
         const bool available = ready & (1 << channel);
         if (available) {
            const uint32_t* src = i2s_capture_buf[ready & 1]; // Source DMA buffer

            const uint8_t offset = (channel == AUDIO_CHANNEL_I2S_LEFT ? 16 : 0);
            for (uint x = 0; x < I2S_SAMPLE_COUNT; x++) {
               int16_t v = (src[x] >> offset) & 0xFFFF;
               printf("%d\n", v);
               buffers[channel][x] = v;
            }
            i2s_end_time_us = buf_i2s_done_time_us; // Cache done time

            buf_i2s_ready &= ~(1 << channel);       // Clear flag
         }

         *capture_end_time_us = i2s_end_time_us;
         *buffer = buffers[channel];
         *samples = I2S_SAMPLE_COUNT;
         return available;
      }
      default:
         *capture_end_time_us = 0;
         *buffer = NULL;
         *samples = 0;
         return false;
   }
}

uint32_t get_capture_duration_us(analog_channel_t channel) {
   switch (channel) {
      case AUDIO_CHANNEL_LEFT:
      case AUDIO_CHANNEL_RIGHT:
      case AUDIO_CHANNEL_MIC:
         return adc_capture_duration_us;
      case AUDIO_CHANNEL_I2S_RIGHT:
      case AUDIO_CHANNEL_I2S_LEFT:
         return i2s_capture_duration_us;
      default:
         return 1;
   }
}

static inline uint32_t log2i(uint32_t n) {
   uint32_t level = 0;
   while (n >>= 1)
      level++;
   return level;
}

static void init_pingpong_dma(const uint channel1, const uint channel2, uint dreq, const volatile void* read_addr, volatile void* write_addr1, volatile void* write_addr2,
                              uint transfer_count, enum dma_channel_transfer_size size, uint irq_num, irq_handler_t handler) {
   // Ring bit must be log2 of total bytes transferred
   const uint32_t ring_bit = log2i(transfer_count * (size == DMA_SIZE_32 ? 4 : (size == DMA_SIZE_16 ? 2 : 1)));

   // Channel 1
   dma_channel_config c1 = dma_channel_get_default_config(channel1);
   channel_config_set_transfer_data_size(&c1, size);

   channel_config_set_read_increment(&c1, false); // read_addr
   channel_config_set_write_increment(&c1, true); // write_addr1

   channel_config_set_ring(&c1, true, ring_bit);  // Wrap write addr every n bits
   channel_config_set_dreq(&c1, dreq);

   channel_config_set_chain_to(&c1, channel2); // Start channel 2 once finshed

   if (irq_num == DMA_IRQ_0) {                 // not: dma_irqn_set_channel_enabled
      dma_channel_set_irq0_enabled(channel1, true);
   } else {
      dma_channel_set_irq1_enabled(channel1, true);
   }

   dma_channel_configure(channel1, &c1, write_addr1, read_addr, transfer_count, false);

   // Channel 2
   dma_channel_config c2 = dma_channel_get_default_config(channel2);
   channel_config_set_transfer_data_size(&c2, size);

   channel_config_set_read_increment(&c2, false); // read_addr
   channel_config_set_write_increment(&c2, true); // write_addr2

   channel_config_set_ring(&c2, true, ring_bit);  // Wrap write addr every n bits
   channel_config_set_dreq(&c2, dreq);

   channel_config_set_chain_to(&c2, channel1); // Start channel 1 once finshed

   if (irq_num == DMA_IRQ_0) {                 // not: dma_irqn_set_channel_enabled
      dma_channel_set_irq0_enabled(channel2, true);
   } else {
      dma_channel_set_irq1_enabled(channel2, true);
   }

   dma_channel_configure(channel2, &c2, write_addr2, read_addr, transfer_count, false);

   // Add IRQ handler
   irq_add_shared_handler(irq_num, handler, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
   irq_set_enabled(irq_num, true);
}

static void dma_channels_abort(uint ch1, uint ch2, uint irq_num) {
   // Errata RP2040-E13
   if (irq_num == DMA_IRQ_0) {
      dma_channel_set_irq0_enabled(ch1, false);
      dma_channel_set_irq0_enabled(ch2, false);
   } else {
      dma_channel_set_irq1_enabled(ch1, false);
      dma_channel_set_irq1_enabled(ch2, false);
   }

   // Abort both linked DMA channels
   dma_hw->abort = (1u << ch1) | (1u << ch2);

   // Wait for all aborts to complete
   while (dma_hw->abort)
      tight_loop_contents();

   // Wait for channels to not be busy
   while (dma_hw->ch[ch1].ctrl_trig & DMA_CH0_CTRL_TRIG_BUSY_BITS)
      tight_loop_contents();

   while (dma_hw->ch[ch2].ctrl_trig & DMA_CH0_CTRL_TRIG_BUSY_BITS)
      tight_loop_contents();
}