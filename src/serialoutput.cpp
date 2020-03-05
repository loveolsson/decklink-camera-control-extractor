// Stolen in parts from https://stackoverflow.com/questions/31663776/ubuntu-c-termios-h-example-program

#include "serialoutput.h"

#include <sys/fcntl.h>    // for open, O_NDELAY, O_NOCTTY, O_RDWR
#include <sys/termios.h>  // for termios, cfmakeraw, cfsetispeed, cfsetospeed
#include <unistd.h>       // for write, close
#include <iostream>       // for operator<<, endl, basic_ostream, cout, ostream


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

SerialOutput::SerialOutput(const char *_deviceName, int _baudRate)
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
    this->fd = open(this->deviceName, O_RDWR | O_NOCTTY | O_NDELAY);
    if (this->fd < 0)
    {
        std::cout << "Error [serial_communcation]: opening Port: " << this->deviceName << std::endl;
        return false;
    }

    //struct termios
    termios serial = {0};

    //get parameters associated with the terminal
    if (tcgetattr(fd, &serial) < 0)
    {
        std::cout << "Error [serial_communication]: getting configuration" << std::endl;
        return false;
    }

    cfsetospeed(&serial, (speed_t)baudrate);
    cfsetispeed(&serial, (speed_t)baudrate);

    // Setting other Port Stuff
    serial.c_cflag &= ~PARENB;
    serial.c_cflag |= CS8; // 8 bits per byte (most common)
    serial.c_cflag &= ~CRTSCTS;
    serial.c_cflag |= CREAD | CLOCAL;
    serial.c_lflag &= ~ICANON;
    serial.c_lflag &= ~ECHO; // Disable echo
    serial.c_lflag &= ~ECHOE; // Disable erasure
    serial.c_lflag &= ~ECHONL; // Disable new-line echo
    serial.c_lflag &= ~ISIG;
    serial.c_iflag &= ~(IXON | IXOFF | IXANY);
    serial.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);
    serial.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
    serial.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
    serial.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
    serial.c_cc[VMIN] = 0;

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
