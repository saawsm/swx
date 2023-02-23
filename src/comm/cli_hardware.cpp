#include "cli_hardware.h"

swx::SpiCli::SpiCli(parse_cb parse, spi_inst_t* spi) : Cli(parse), spi(spi) {}

void swx::SpiCli::writeBlocking(const void* data, uint8_t size) {
   spi_write_blocking(spi, (const uint8_t*)data, size);
}

int swx::SpiCli::readBlocking(uint8_t* data, uint8_t size) {
   return spi_read_blocking(spi, 0, data, size);
}

bool swx::SpiCli::isReadable() const {
   return spi_is_readable(spi);
}

swx::UartCli::UartCli(parse_cb parse, uart_inst_t* uart) : Cli(parse), uart(uart) {}

void swx::UartCli::writeBlocking(const void* data, uint8_t size) {
   uart_write_blocking(uart, (const uint8_t*)data, size);
}

int swx::UartCli::readBlocking(uint8_t* data, uint8_t size) {
   uart_read_blocking(uart, data, size);
   return size;
}

bool swx::UartCli::isReadable() const {
   return uart_is_readable(uart);
}