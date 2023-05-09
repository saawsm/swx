#include "analog_capture.h"

#include <pico/stdlib.h>

#include <hardware/dma.h>
#include <hardware/irq.h>

#include <hardware/adc.h>

#define PIN_ADC_BASE (26)

#define ADC_CAPTURE_DEPTH (1024)
#define ADC_CAPTURE_RING_BITS (10)

#define ADC_SAMPLES_PER_SECOND (62500)
#define SAMPLED_ADC_CHANNELS (3)

static void init_pingpong_dma(const uint channel1, const uint channel2, uint dreq, const volatile void* read_addr, volatile void* write_addr1, volatile void* write_addr2,
                              uint transfer_count, enum dma_channel_transfer_size size, uint ring_bit, uint irq_num, irq_handler_t handler);
static void irq();

static uint dma_adc_channel1;
static uint dma_adc_channel2;

// DMA Buffers
static uint8_t capture_adc_buf1[ADC_CAPTURE_DEPTH] __attribute__((aligned(2048)));
static uint8_t capture_adc_buf2[ADC_CAPTURE_DEPTH] __attribute__((aligned(2048)));

static const uint8_t* capture_buf_lookup[] = {capture_adc_buf1, capture_adc_buf2};

// Consumer buffers, updated on request
static uint8_t adc_buffers[TOTAL_ANALOG_CHANNELS][ADC_CAPTURE_DEPTH / SAMPLED_ADC_CHANNELS] = {0};

// Lookup Table: Analog channel -> ADC round robin stripe offset
static const uint8_t adc_stripe_lookup[] = {
    [AUDIO_CHANNEL_MIC] = (PIN_AUDIO_MIC - PIN_ADC_BASE),
    [AUDIO_CHANNEL_LEFT] = (PIN_AUDIO_LEFT - PIN_ADC_BASE),
    [AUDIO_CHANNEL_RIGHT] = (PIN_AUDIO_RIGHT - PIN_ADC_BASE),
};

static volatile uint16_t adc_buf_ready;        // Used in irq: The adc buffer that finished capture, now ready for reading/processing
static volatile uint32_t adc_buf_done_time_us; // Used in irq: The absolute time the capture finished in microseconds
static uint32_t adc_end_time_us;               // Cached version of: adc_buf_done_time_us

static const uint32_t adc_capture_duration_us = ADC_CAPTURE_DEPTH * (1000000ul / ADC_SAMPLES_PER_SECOND);

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
                  true   // Reduce samples to 8 bits
   );

   uint32_t divider = 48000000 / ADC_SAMPLES_PER_SECOND;
   adc_set_clkdiv(divider - 1);

   // Setup ping-pong DMA for ADC FIFO writing to capture_buf, wrapping once filled
   dma_adc_channel1 = dma_claim_unused_channel(true);
   dma_adc_channel2 = dma_claim_unused_channel(true);
   init_pingpong_dma(dma_adc_channel1, dma_adc_channel2, DREQ_ADC, &adc_hw->fifo, capture_adc_buf1, capture_adc_buf2, ADC_CAPTURE_DEPTH, DMA_SIZE_8,
                     ADC_CAPTURE_RING_BITS, DMA_IRQ_0, irq);

   // Start channel 1
   adc_buf_ready = 0;
   dma_channel_start(dma_adc_channel1);
}

void analog_capture_free() {
   LOG_DEBUG("Free analog capture...\n");

   analog_capture_stop();

   // Errata RP2040-E13
   dma_channel_set_irq0_enabled(dma_adc_channel1, false);
   dma_channel_set_irq0_enabled(dma_adc_channel2, false);

   // Abort both linked DMA channels
   dma_hw->abort = (1u << dma_adc_channel1) | (1u << dma_adc_channel2);

   // Wait for all aborts to complete
   while (dma_hw->abort)
      tight_loop_contents();

   // Wait for channels to not be busy
   while (dma_hw->ch[dma_adc_channel1].ctrl_trig & DMA_CH0_CTRL_TRIG_BUSY_BITS)
      tight_loop_contents();

   while (dma_hw->ch[dma_adc_channel2].ctrl_trig & DMA_CH0_CTRL_TRIG_BUSY_BITS)
      tight_loop_contents();

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

static void __not_in_flash_func(irq)() {
   if (dma_channel_get_irq0_status(dma_adc_channel1)) {
      adc_select_input(0);

      adc_buf_ready = 0x3fff | (0 << 14); // lookup index: 0
      adc_buf_done_time_us = time_us_32();

      dma_channel_acknowledge_irq0(dma_adc_channel1);
   } else if (dma_channel_get_irq0_status(dma_adc_channel2)) {
      adc_select_input(0);

      adc_buf_ready = 0x3fff | (1 << 14); // lookup index: 1
      adc_buf_done_time_us = time_us_32();

      dma_channel_acknowledge_irq0(dma_adc_channel2);
   }
}

bool fetch_analog_buffer(analog_channel_t channel, uint16_t* samples, uint8_t** buffer, uint32_t* capture_end_time_us) {
   if (!channel || channel > TOTAL_ANALOG_CHANNELS)
      return false;

   const uint16_t index = channel - 1;
   const uint16_t buf_ready = adc_buf_ready; // local copy, since volatile

   // Check if processed buffer needs to be updated
   const bool needs_update = buf_ready & (1 << channel);

   // get filled capture buffer
   const uint8_t* src = capture_buf_lookup[(buf_ready & (3 << 14)) >> 14];
   switch (channel) {
      case AUDIO_CHANNEL_LEFT:
      case AUDIO_CHANNEL_RIGHT:
      case AUDIO_CHANNEL_MIC: {
         if (needs_update) {                                  // update cached buffer from DMA capture buffer
            adc_end_time_us = adc_buf_done_time_us;
            for (uint x = 0; x < sizeof(adc_buffers[0]); x++) // unravel interleaved capture buffer
               adc_buffers[index][x] = src[(x * SAMPLED_ADC_CHANNELS) + adc_stripe_lookup[channel]];
         }

         *capture_end_time_us = adc_end_time_us;

         *buffer = adc_buffers[index];
         *samples = sizeof(adc_buffers[0]);
         break;
      }
      default:
         return false;
   }

   /*
   #ifndef NDEBUG
      if (buf_ready != adc_buf_ready)
         LOG_WARN("Analog capture IRQ occurred during fetch!\n");
   #endif
   */

   if (needs_update) // clear flag
      adc_buf_ready &= ~(1 << channel);

   return needs_update;
}

uint32_t get_capture_duration_us(analog_channel_t channel) {
   switch (channel) {
      case AUDIO_CHANNEL_LEFT:
      case AUDIO_CHANNEL_RIGHT:
      case AUDIO_CHANNEL_MIC: {
         return adc_capture_duration_us;
      }
      default:
         return 1;
   }
}

static void init_pingpong_dma(const uint channel1, const uint channel2, uint dreq, const volatile void* read_addr, volatile void* write_addr1, volatile void* write_addr2,
                              uint transfer_count, enum dma_channel_transfer_size size, uint ring_bit, uint irq_num, irq_handler_t handler) {
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