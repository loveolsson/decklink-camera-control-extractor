#pragma once
#include <stddef.h>  // for size_t
#include <stdint.h>  // for uint8_t

class SerialOutput
{
public:
    SerialOutput(const char *_deviceName, int _baudRate);
    ~SerialOutput();

    bool Begin();
    void Write(uint8_t *data, size_t size);

private:
    const char *deviceName;
    const int baudrate;
    int fd = -1;
};