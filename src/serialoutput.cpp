// Stolen in parts from https://stackoverflow.com/questions/31663776/ubuntu-c-termios-h-example-program

#include "serialoutput.h"

#include "crc.h"
#include "defines.h"

#include <stddef.h>       // for size_t
#include <stdint.h>       // for uint8_t
#include <sys/fcntl.h>    // for open, O_NDELAY, O_NOCTTY, O_RDWR
#include <sys/termios.h>  // for termios, cfmakeraw, cfsetispeed, cfsetospeed
#include <unistd.h>       // for write, close

#include <array>
#include <iostream>  // for operator<<, endl, basic_ostream, cout, ostream

SerialOutput::SerialOutput(const std::string &_deviceName, int _baudRate)
    : deviceName(_deviceName)
    , baudrate(_baudRate)
{
}

SerialOutput::~SerialOutput()
{
    if (this->fd >= 0) {
        std::cout << "Closing serial port...";
        if (close(fd) == 0) {
            std::cout << "success." << std::endl;
        } else {
            std::cout << "failed." << std::endl;
        }
    }
}

bool
SerialOutput::Begin()
{
    this->fd = open(this->deviceName.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (this->fd < 0) {
        std::cout << "Error [serial_communcation]: opening Port: " << this->deviceName << std::endl;
        return false;
    }

    //struct termios
    termios serial = {};

    //get parameters associated with the terminal
    if (tcgetattr(fd, &serial) < 0) {
        std::cout << "Error [serial_communication]: getting configuration" << std::endl;
        return false;
    }

    cfsetospeed(&serial, (speed_t)baudrate);
    cfsetispeed(&serial, (speed_t)baudrate);

    /* 8N1 Mode */
    serial.c_cflag &= ~PARENB;   // Disables the Parity Enable bit(PARENB),So No Parity
    serial.c_cflag &= ~CSTOPB;   // CSTOPB = 2 Stop bits,here it is cleared so 1 Stop bit
    serial.c_cflag &= ~CSIZE;    // Clears the mask for setting the data size
    serial.c_cflag |= CS8;       // Set the data bits = 8
    serial.c_cflag &= ~CRTSCTS;  // No Hardware flow Control
    serial.c_cflag &= ~CREAD;    // Disable receiver
    serial.c_cflag |= CLOCAL;    // Ignore Modem Control lines

    serial.c_iflag &= ~(IXON | IXOFF | IXANY);  // Disable XON/XOFF flow control both i/p and o/p
    serial.c_iflag &= ~(ICANON | ECHO | ECHOE | ISIG);  // Non Cannonical mode

    //serial.c_oflag &= ~OPOST; // No Output Processing
    serial.c_oflag &= ~(OPOST | /*OLCUC |*/ ONLCR | OCRNL | ONLRET | OFDEL);

    // Setting Time outs
    serial.c_cc[VMIN]  = 10;  // Read at least 10 characters
    serial.c_cc[VTIME] = 0;   // Wait indefinetly
    // Set attributes to port
    if (tcsetattr(this->fd, TCSANOW, &serial) < 0) {
        std::cout << "Error [serial_communication]: set attributes" << std::endl;
        return false;
    }

    return true;
}

void
SerialOutput::Write(const Packet &pkt)
{
    if (this->fd < 0) {
        return;
    }

    const size_t totalLength = pkt.header.GetTotalSize();

    const MasterHeader masterHeader = {
        (uint8_t)totalLength,
        CRC((uint8_t *)&pkt, totalLength),
    };

    if (write(fd, leadInBytes.data(), leadInBytes.size()) < 0) {
        std::cout << "Failed to write leadInBytes" << std::endl;
        return;
    }

    if (write(fd, &masterHeader, sizeof(MasterHeader)) < 0) {
        std::cout << "Failed to write MasterHeader" << std::endl;
        return;
    }

    if (write(fd, &pkt, totalLength) < 0) {
        std::cout << "Failed to write Packet" << std::endl;
        return;
    }
}
