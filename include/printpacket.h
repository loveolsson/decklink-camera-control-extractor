#pragma once
#include <stddef.h>  // for size_t
#include <stdint.h>  // for uint8_t
#include <string>    // for string
struct Packet;

std::string PrintPacket(Packet &pkt);
std::string ToHex(const uint8_t *buffer, size_t size);
