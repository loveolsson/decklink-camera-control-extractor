// Stolen in parts from https://stackoverflow.com/questions/31663776/ubuntu-c-termios-h-example-program

#include "serialoutput.h"
#include <termios.h>
#include <iostream>
#include <string>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

static uint8_t
CRC(uint8_t *data, size_t length)
{
    uint8_t sum = 0;

    for (size_t i = 0; i < length; ++i)
    {
        sum ^= data[i];
    }

    return sum;
}

SerialOutput::SerialOutput(const std::string &_deviceName, int _baudRate)
    : deviceName(_deviceName), baudrate(_baudRate)
{
}

SerialOutput::~SerialOutput()
{
    if (this->fd >= 0)
    {
        std::cout << "Closing serial port...";
        if (close(fd) == 0)
        {
            std::cout << "success." << std::endl;
        }
        else
        {
            std::cout << "failed." << std::endl;
        }
    }
}

bool SerialOutput::Begin()
{
    this->fd = open(this->deviceName.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (this->fd < 0)
    {
        std::cout << "Error [serial_communcation]: opening Port: " << this->deviceName << std::endl;
        return false;
    }

    //struct termios
    termios serial;

    //get parameters associated with the terminal
    if (tcgetattr(fd, &serial) < 0)
    {
        std::cout << "Error [serial_communication]: getting configuration" << std::endl;
        return false;
    }

    cfsetospeed(&serial, (speed_t)baudrate);
    cfsetispeed(&serial, (speed_t)baudrate);

    // Setting other Port Stuff
    serial.c_cflag &= ~PARENB;        // Make 8n1
    serial.c_cflag &= ~CSTOPB;        //
    serial.c_cflag &= ~CSIZE;         //
    serial.c_cflag |= CS8;            //
    serial.c_cflag &= ~CRTSCTS;       // no flow control
    serial.c_cc[VMIN] = 1;            // read doesn't block
    serial.c_cc[VTIME] = 5;           // 0.5 seconds read timeout
    serial.c_cflag |= CREAD | CLOCAL; // turn on READ & ignore ctrl lines

    /* Make raw */
    cfmakeraw(&serial);

    /* Flush Port, then applies attributes */
    tcflush(this->fd, TCIFLUSH);

    //set attributes to port
    if (tcsetattr(this->fd, TCSANOW, &serial) < 0)
    {
        std::cout << "Error [serial_communication]: set attributes" << std::endl;
        return false;
    }

    return true;
}

void SerialOutput::Write(uint8_t *data, size_t size)
{
    if (this->fd < 0)
    {
        return;
    }

    const uint8_t leadIn[] = {
        254,             // The receiver is looking for 3 bytes of 254 in a row to start parsing
        254,             //
        254,             //
        (uint8_t)size,   // Size of packet, including header
        CRC(data, size), // XOR CRC
    };

    //attempt to send
    if (write(fd, leadIn, sizeof(leadIn)) < 0)
    {
        std::cout << "Failed to write lead-in" << std::endl;
        return;
    }

    //attempt to send
    if (write(fd, data, size) < 0)
    {
        std::cout << "Failed to write packet" << std::endl;
        return;
    }
}
