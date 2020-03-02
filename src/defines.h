#pragma once
#include <cstdint>

#define NUM(a) (sizeof(a) / sizeof(*a))
#define PADDING(x) (((x + 3) / 4) * 4)

#pragma pack(1)
struct Header {
  uint8_t dest;
  uint8_t len;
  uint8_t command;
  uint8_t _reserved;
};

struct CommandInfo {
  uint8_t category;
  uint8_t parameter;
  uint8_t type;
};

struct Packet {
  Header header;
  CommandInfo commandInfo;
  uint8_t data[64 - sizeof(Header) - sizeof(CommandInfo)];
};
#pragma pack(pop)