#pragma once
#include <stdint.h>

#include <cstddef>

void InitUART();
size_t UARTAvailable();
int UARTReadBytes(uint8_t *data, const size_t size);
uint8_t UARTReadOneByte();
