#ifndef _CLI_HARDWARE_H
#define _CLI_HARDWARE_H

#include "cli.h"

#include <hardware/spi.h>
#include <hardware/uart.h>

namespace swx {

   class SpiCli final : public Cli {
      spi_inst_t* const spi;

   public:
      SpiCli(parse_cb parse, spi_inst_t* spi);

      virtual void writeBlocking(const void* data, uint8_t size) override;

      virtual int readBlocking(uint8_t* data, uint8_t size) override;

      virtual bool isReadable() const override;
   };

   class UartCli final : public Cli {
      uart_inst_t* const uart;

   public:
      UartCli(parse_cb parse, uart_inst_t* uart);

      virtual void writeBlocking(const void* data, uint8_t size) override;

      virtual int readBlocking(uint8_t* data, uint8_t size) override;

      virtual bool isReadable() const override;
   };

} // namespace swx

#endif // _CLI_HARDWARE_H