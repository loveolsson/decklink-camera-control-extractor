#pragma once
struct Packet;

class SerialOutput
{
public:
    SerialOutput(const char *_deviceName, int _baudRate);
    ~SerialOutput();

    bool Begin();
    void Write(Packet &pkt);

private:
    const char *deviceName;
    const int baudrate;
    int fd = -1;
};