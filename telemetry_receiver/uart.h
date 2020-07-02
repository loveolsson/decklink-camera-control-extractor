#pragma once
#include <stdint.h>

#include <cstddef>

void InitUART();
size_t UARTAvailable();
uint8_t UARTReadBytes(uint8_t *data, const size_t size);
uint8_t UARTReadOneByte();
