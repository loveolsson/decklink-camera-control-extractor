#pragma once
#include <iosfwd>

struct Packet;

class SerialOutput
{
public:
    SerialOutput(const std::string &_deviceName, int _baudRate);
    ~SerialOutput();

    bool Begin();
    void Write(const Packet &pkt);

private:
    const std::string &deviceName;
    const int baudrate;
    int fd = -1;
};