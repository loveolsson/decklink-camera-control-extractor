#pragma once
#include <string>

class SerialOutput
{
public:
    SerialOutput(const std::string &_deviceName, int _baudRate);
    ~SerialOutput();

    bool Begin();

    void Write(uint8_t *data, size_t size);

private:

    const std::string deviceName;
    const int baudrate;
    int fd = -1;
};