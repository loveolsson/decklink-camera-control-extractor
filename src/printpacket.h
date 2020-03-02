#pragma once
#include "defines.h"
#include <string>

void PrintPacket(Packet &pkt);
std::string ToHex(const uint8_t *buffer, size_t size);
