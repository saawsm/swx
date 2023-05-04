#include "protocol.h"

#include <hardware/spi.h>
#include <hardware/uart.h>

#include "output.h"

#define REPLY_MSG(...)                                                                                                                                                   \
   do {                                                                                                                                                                  \
      const uint8_t data[] = {MSG_ID_MASK | (ctrl), __VA_ARGS__};                                                                                                        \
      write_blocking(origin, &data, sizeof(data));                                                                                                                       \
   } while (0)

static uint16_t lastPulseWidth = 100;

void parse_message(msg_ch_t origin, uint8_t ctrl);

void protocol_process(msg_ch_t src) {
   int mask = 1;
   uint8_t ctrl;

   while (src) { // check each bit flag
      msg_ch_t ch = src & mask;

      if (is_readable(ch) && read_blocking(ch, &ctrl, 1))
         parse_message(ch, ctrl);

      src &= ~mask;
      mask <<= 1;
   }
}

void parse_message(msg_ch_t origin, uint8_t ctrl) {
   extern channel_t channels[CHANNEL_COUNT];

   // const bool has_id = (ctrl & MSG_ID_MASK);
   const bool write = (ctrl & MSG_MODE_MASK);
   const uint8_t mp = (ctrl & MSG_MP_MASK) >> MSG_MP;
   const uint8_t cmd = (ctrl & MSG_CMD_MASK) >> MSG_CMD;

   switch (cmd) {
      case MSG_CMD_STATUS: { // SWX Status - RO
         LOG_FINE_MSG("MSG_CMD_STATUS: VER: %u, CHC: %u\n", SWX_VERSION, CHANNEL_COUNT);

         REPLY_MSG(SWX_VERSION >> 8, SWX_VERSION & 0xff, CHANNEL_COUNT);
         break;
      }

      case MSG_CMD_PSU: { // Power Supply State - R/W
         bool enabled;

         if (write) {
            enabled = mp > 0;
            set_psu_enabled(enabled);
         } else {
            enabled = is_psu_enabled();
            REPLY_MSG(enabled);
         }

         LOG_FINE_MSG("MSG_CMD_PSU: W:%u, VAL:%u\n", write, enabled);
         break;
      }

      case MSG_CMD_CH_STATUS: { // Channel Status - RO
         LOG_FINE_MSG("MSG_CMD_CH_STATUS...\n");
         if (mp < CHANNEL_COUNT) {
            const uint8_t status = channels[mp].status;

            LOG_FINE_MSG("MSG_CMD_CH_STATUS: MP:%u, VAL:%u\n", mp, status);
            REPLY_MSG(mp, status);
         }
         break;
      }

      case MSG_CMD_CH_EN: { // Channel Enabled State (R/W)
         LOG_FINE_MSG("MSG_CMD_CH_EN...\n");

         bool enabled;
         uint16_t tod_ms;

         if (write) {
            uint8_t buffer[1];
            read_blocking(origin, buffer, sizeof(buffer));

            enabled = buffer[0];
            tod_ms = 0; // TODO: Add support for turn_off_delay_ms for MSG_CMD_CH_EN

            output_set_gen_enabled(mp, enabled, tod_ms);
         } else {
            if (mp >= CHANNEL_COUNT)
               break;

            enabled = channels[mp].gen_enabled;

            REPLY_MSG(mp, enabled);
         }

         LOG_FINE_MSG("MSG_CMD_CH_EN: W:%u MP:%u, VAL:%u, TOD:%u\n", write, mp, enabled, tod_ms);
         break;
      }

      case MSG_CMD_CH_POWER: { // Channel Power Level Percent (R/W)
         LOG_FINE_MSG("MSG_CMD_CH_POWER...\n");

         uint16_t val;

         if (write) {
            uint8_t buffer[2];
            read_blocking(origin, buffer, sizeof(buffer));

            if (mp >= CHANNEL_COUNT)
               break;

            val = (buffer[0] << 8 | buffer[1]);

            channels[mp].power_level = val;
         } else {
            if (mp >= CHANNEL_COUNT)
               break;

            val = channels[mp].power_level;
            REPLY_MSG(mp, HL16(val));
         }

         LOG_FINE_MSG("MSG_CMD_CH_POWER: W:%u MP:%u VAL:%u\n", write, mp, val);
         break;
      }

      case MSG_CMD_CH_PARAM: { // Dynamic Channel Parameter (R/W)
         LOG_FINE_MSG("MSG_CMD_CH_PARAM...\n");

         uint8_t param;
         uint8_t target;
         uint16_t val = 0;

         if (write) {
            uint8_t buffer[3];
            read_blocking(origin, buffer, sizeof(buffer));

            param = buffer[0] >> 4;
            target = buffer[0] & 0xf;

            val = (buffer[1] << 8 | buffer[2]);

            if (mp < CHANNEL_COUNT && param < TOTAL_PARAMS && target < TOTAL_TARGETS) 
               parameter_set(&channels[mp].parameters[param], target, val);
            
         } else {
            uint8_t buffer[1];
            read_blocking(origin, buffer, sizeof(buffer));

            param = buffer[0] >> 4;
            target = buffer[0] & 0xf;

            if (mp < CHANNEL_COUNT && param < TOTAL_PARAMS && target < TOTAL_TARGETS) {
               val = channels[mp].parameters[param].values[target];
               REPLY_MSG(mp, HL16(val));
            }
         }

         LOG_FINE_MSG("MSG_CMD_CH_PARAM: W:%u MP:%u, P:%u, T:%u VAL:%u\n", write, mp, param, target, val);
         break;
      }

      case MSG_CMD_CH_AI_SRC: { // Channel Audio Source (R/W)
         break;
      }

      case MSG_CMD_CH_LL_PULSE: { // Channel Low Level Pulse - R/W
         LOG_FINE_MSG("MSG_CMD_CH_LL_PULSE...\n");

         if (write) {
            uint8_t buffer[2];
            read_blocking(origin, buffer, sizeof(buffer));

            lastPulseWidth = (buffer[0] << 8 | buffer[1]);
         }

         LOG_FINE_MSG("MSG_CMD_CH_LL_PULSE: W:%u MP:%u, PW:%u\n", write, mp, lastPulseWidth);

         output_pulse(mp, lastPulseWidth, lastPulseWidth, time_us_32());
         break;
      }

      case MSG_CMD_CH_LL_POWER: { // Channel Low Level Set Power - WO
         LOG_FINE_MSG("MSG_CMD_CH_LL_POWER...\n");

         uint8_t buffer[2];
         read_blocking(origin, buffer, sizeof(buffer));

         const uint16_t val = (buffer[0] << 8 | buffer[1]);

         LOG_FINE_MSG("MSG_CMD_CH_LL_POWER: MP:%u, PWR:%u\n", mp, val);
         output_set_power(mp, val);
         break;
      }

      case MSG_CMD_AI_READ: { // Read Audio Input (RO)
         break;
      }

      default:
         break;
   }
}

bool is_readable(msg_ch_t src) {
   switch (src) {
#ifdef SPI_PORT
      case MSG_CH_SPI:
         return spi_is_readable(SPI_PORT);
#endif
#ifdef UART_PORT
      case MSG_CH_UART:
         return uart_is_readable(UART_PORT);
#endif
      case MSG_CH_STDIO: {
         const int c = getchar_timeout_us(0);
         if (c != PICO_ERROR_TIMEOUT) {
            ungetc(c, stdin);
            return true;
         }
         return false;
      }
      default:
         return false;
   }
}

void write_blocking(msg_ch_t dst, const void* data, uint8_t size) {
   int mask = 1;
   while (dst) { // check each bit flag

      switch (dst & mask) {
#ifdef SPI_PORT
         case MSG_CH_SPI: {
            spi_write_blocking(SPI_PORT, (const uint8_t*)data, size);
            break;
         }
#endif
#ifdef UART_PORT
         case MSG_CH_UART: {
            uart_write_blocking(UART_PORT, (const uint8_t*)data, size);
            break;
         }
#endif
         case MSG_CH_STDIO: {
            fwrite(data, 1, size, stdout);
            fflush(stdout);
            break;
         }
         default:
            break;
      }

      dst &= ~mask;
      mask <<= 1;
   }
}

int read_blocking(msg_ch_t src, uint8_t* data, uint8_t size) {
   switch (src) {
#ifdef SPI_PORT
      case MSG_CH_SPI:
         return spi_read_blocking(SPI_PORT, 0, data, size);
#endif
#ifdef UART_PORT
      case MSG_CH_UART: {
         uart_read_blocking(UART_PORT, data, size);
         return size;
      }
#endif
      case MSG_CH_STDIO:
         return fread(data, 1, size, stdin);
      default:
         return 0;
   }
}
