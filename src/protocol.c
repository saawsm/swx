#include "protocol.h"

#include <hardware/spi.h>
#include <hardware/uart.h>

#include "output.h"

#define REPLY_MSG(...)                                                                                                                                                   \
   do {                                                                                                                                                                  \
      const uint8_t data[] = {MSG_ID_BIT | (ctrl), __VA_ARGS__};                                                                                                         \
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
   if (!(ctrl & MSG_ID_BIT)) // Discard any messages wihtout the ID bit set
      return;

   const bool write = (ctrl & MSG_MODE_MASK);
   const uint8_t mp = (ctrl & MSG_MP_MASK) >> MSG_MP;
   const uint8_t cmd = (ctrl & MSG_CMD_MASK) >> MSG_CMD;

   if (cmd >= MSG_CMD_CH_STATUS && mp >= CHANNEL_COUNT) {
      LOG_WARN("Message has invalid channel number: cmd=%u ch=%u - Ignoring...\n", cmd, mp);
      return;
   }

   extern channel_t channels[CHANNEL_COUNT];
   switch (cmd) {
      case MSG_CMD_STATUS: { // SWX Status - RO
         LOG_DEBUG("MSG_CMD_STATUS: version=%u channels=%u\n", SWX_VERSION, CHANNEL_COUNT);

         REPLY_MSG(SWX_VERSION >> 8, SWX_VERSION & 0xff, CHANNEL_COUNT);
         break;
      }

      case MSG_CMD_PSU: { // Power Supply State - R/W
         bool enabled;
         if (write) {
            enabled = mp > 0;
            set_psu_enabled(enabled);
         } else { // read
            enabled = is_psu_enabled();
            REPLY_MSG(enabled);
         }

         LOG_DEBUG("MSG_CMD_PSU: write=%u value=%u\n", write, enabled);
         break;
      }

      case MSG_CMD_CH_STATUS: { // Channel Status - RO
         const uint8_t status = channels[mp].status;

         REPLY_MSG(mp, status);

         LOG_DEBUG("MSG_CMD_CH_STATUS: ch=%u value=%u\n", mp, status);
         break;
      }

      case MSG_CMD_CH_EN: { // Channel Enabled State (R/W)
         bool enabled = false;
         uint16_t tod_ms = 0;

         if (write) {
            uint8_t buffer[1];
            read_blocking(origin, buffer, sizeof(buffer));

            enabled = buffer[0];
            tod_ms = 0; // TODO: Add support for turn_off_delay_ms for MSG_CMD_CH_EN

            output_set_gen_enabled(mp, enabled, tod_ms);
         } else { // read
            enabled = channels[mp].gen_enabled;

            REPLY_MSG(mp, enabled);
         }

         LOG_DEBUG("MSG_CMD_CH_EN: write=%u ch=%u value=%u tod=%u\n", write, mp, enabled, tod_ms);
         break;
      }

      case MSG_CMD_CH_POWER: { // Channel Power Level Percent (R/W)
         uint16_t val = 0;

         if (write) {
            uint8_t buffer[2];
            read_blocking(origin, buffer, sizeof(buffer));

            val = (buffer[0] << 8 | buffer[1]);
            channels[mp].power_level = val;
         } else { // read
            val = channels[mp].power_level;
            REPLY_MSG(mp, HL16(val));
         }

         LOG_DEBUG("MSG_CMD_CH_POWER: write=%u ch=%u value=%u\n", write, mp, val);
         break;
      }

      case MSG_CMD_CH_PARAM: { // Dynamic Channel Parameter (R/W)
         uint8_t buffer[3];
         read_blocking(origin, buffer, write ? sizeof(buffer) : 1);

         const uint8_t param = buffer[0] >> 4;
         const uint8_t target = buffer[0] & 0xf;

         if (param < TOTAL_PARAMS && target < TOTAL_TARGETS) {

            uint16_t val = 0;

            if (write) {
               val = (buffer[1] << 8 | buffer[2]);
               parameter_set(&channels[mp].parameters[param], target, val);
            } else { // read
               val = channels[mp].parameters[param].values[target];
               REPLY_MSG(mp, buffer[0], HL16(val));
            }

            LOG_DEBUG("MSG_CMD_CH_PARAM: write=%u ch=%u param=%u target=%u value=%u\n", write, mp, param, target, val);
         } else {
            LOG_WARN("MSG_CMD_CH_PARAM: write=%u param=%u target=%u - Parameter or target invalid! Ignoring...\n", write, param, target);
         }
         break;
      }

      case MSG_CMD_CH_AI_SRC: { // Channel Audio Source (R/W)
         uint8_t val = 0;
         if (write) {
            uint8_t buffer[1];
            read_blocking(origin, buffer, sizeof(buffer));

            val = buffer[0];
            channels[mp].audio_src = val;
         } else { // read
            val = channels[mp].audio_src;
            REPLY_MSG(mp, val);
         }

         LOG_DEBUG("MSG_CMD_CH_AI_SRC: write=%u ch=%u value=%u\n", write, mp, val);
         break;
      }

      case MSG_CMD_CH_LL_PULSE: { // Channel Low Level Pulse - R/W
         if (write) {
            uint8_t buffer[2];
            read_blocking(origin, buffer, sizeof(buffer));

            lastPulseWidth = (buffer[0] << 8 | buffer[1]);
         }

         output_pulse(mp, lastPulseWidth, lastPulseWidth, time_us_32());

         LOG_DEBUG("MSG_CMD_CH_LL_PULSE: write=%u ch=%u value=%u\n", write, mp, lastPulseWidth);
         break;
      }

      case MSG_CMD_CH_LL_POWER: { // Channel Low Level Set Power - WO
         uint8_t buffer[2];
         read_blocking(origin, buffer, sizeof(buffer));

         const uint16_t val = (buffer[0] << 8 | buffer[1]);

         output_set_power(mp, val);

         LOG_DEBUG("MSG_CMD_CH_LL_POWER: ch=%u value=%u\n", mp, val);
         break;
      }

      case MSG_CMD_AI_READ: { // Read Audio Input (RO)
         break;
      }

      case MSG_CMD_CH_PARAM_FLAGS: { // Channel Parameter State Flags (R/W)
         uint8_t buffer[2];
         read_blocking(origin, buffer, write ? sizeof(buffer) : 1);

         const uint8_t param = buffer[0] >> 4;

         if (param < TOTAL_PARAMS) {
            parameter_t* p = &channels[mp].parameters[param];
            REPLY_MSG(mp, buffer[0], p->flags);

            LOG_DEBUG("MSG_CMD_CH_PARAM_FLAGS: write=%u ch=%u param=%u flags=%u clear=%u\n", write, mp, param, p->flags, buffer[1]);

            if (write) // clear bits
               p->flags &= ~buffer[1];

         } else {
            LOG_WARN("MSG_CMD_CH_PARAM_FLAGS: write=%u ch=%u param=%u clear=%u - Parameter invalid! Ignoring...\n", write, mp, param, buffer[1]);
         }
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
