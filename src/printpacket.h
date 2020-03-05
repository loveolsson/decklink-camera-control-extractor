#pragma once
#include <stddef.h>  // for size_t
#include <stdint.h>  // for uint8_t
#include <iosfwd>    // for string
struct Packet;

void PrintPacket(Packet &pkt);
std::string ToHex(const uint8_t *buffer, size_t size);
