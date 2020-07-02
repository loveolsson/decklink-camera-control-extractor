#pragma once

#include <stddef.h>

#include <array>
#include <cstdint>

#define NUM(a) (sizeof(a) / sizeof(*a))
#define PKT_PADDING(x) (((x + 3) / 4) * 4)

static const std::array<uint8_t, 3> leadInBytes = {'B', 'M', 'E'};

#pragma pack(push, 1)
struct MasterHeader {
    uint8_t size;
    uint8_t crc;
};

struct CommandInfo {
    uint8_t category;
    uint8_t parameter;
    uint8_t type;
};

struct Header {
    uint8_t dest;
    uint8_t len;
    uint8_t command;
    uint8_t _reserved;

    size_t
    GetSize() const
    {
        return this->len;
    }

    size_t
    GetPaddedSize() const
    {
        return PKT_PADDING(this->len);
    }

    size_t
    GetDataSize() const
    {
        return this->len - sizeof(CommandInfo);
    }

    size_t
    GetTotalSize() const
    {
        return sizeof(Header) + this->len;
    }

    size_t
    GetPaddedTotalSize() const
    {
        return PKT_PADDING(sizeof(Header) + this->len);
    }
};

struct Packet {
    Header header;
    CommandInfo commandInfo;
    uint8_t data[64 - sizeof(Header) - sizeof(CommandInfo)];
};
#pragma pack(pop)

static_assert(leadInBytes.size() == 3, "Size of leadInBytes is not correct");
static_assert(sizeof(MasterHeader) == 2, "Size of MasterHeader is not correct");
static_assert(sizeof(Header) == 4, "Size of Header is not correct");
static_assert(sizeof(CommandInfo) == 3, "Size of CommandInfo is not correct");
static_assert(sizeof(Packet) == 64, "Size of Packet is not correct");
