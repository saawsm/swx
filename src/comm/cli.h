#ifndef _CLI_H
#define _CLI_H

#include "../swx.h"

#include <initializer_list>

#define RES_START (245)

namespace swx {

   class Cli;

   typedef void (*parse_cb)(Cli& cli, uint8_t ctrl);

   class Cli {
      const parse_cb parse;

   public:
      Cli(parse_cb parse);
      virtual ~Cli() = default;

      bool process();

      void writeBlocking(std::initializer_list<uint8_t> data);
      
      virtual void writeBlocking(const void* data, uint8_t size);

      virtual int readBlocking(uint8_t* data, uint8_t size);

      virtual bool isReadable() const;

   };

} // namespace swx

#endif // _CLI_H