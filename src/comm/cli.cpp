#include "cli.h"

#include <pico/stdio.h>

swx::Cli::Cli(parse_cb parse) : parse(parse) {}

bool swx::Cli::process() {
   if (!isReadable())
      return false;

   uint8_t ctrl;
   readBlocking(&ctrl, 1);

   parse(*this, ctrl);
   return true;
}

void swx::Cli::writeBlocking(std::initializer_list<uint8_t> data) {
   writeBlocking(data.begin(), data.size());
}

void swx::Cli::writeBlocking(const void* data, uint8_t size) {
   fwrite(data, 1, size, stdout);
   fflush(stdout);
}

int swx::Cli::readBlocking(uint8_t* data, uint8_t size) {
   return fread(data, 1, size, stdin);
}

bool swx::Cli::isReadable() const {
   int c = getchar_timeout_us(0);
   if (c != PICO_ERROR_TIMEOUT) {
      ungetc(c, stdin);
      return true;
   }
   return false;
}