// Stolen in parts from https://stackoverflow.com/questions/31663776/ubuntu-c-termios-h-example-program

#include "serialoutput.h"

#include <sys/fcntl.h>   // for open, O_NDELAY, O_NOCTTY, O_RDWR
#include <sys/termios.h> // for termios, cfmakeraw, cfsetispeed, cfsetospeed
#include <unistd.h>      // for write, close
#include <iostream>      // for operator<<, endl, basic_ostream, cout, ostream
#include <thread>        // for operator<<, endl, basic_ostream, cout, ostream

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

    /* 8N1 Mode */
    serial.c_cflag &= ~PARENB; /* Disables the Parity Enable bit(PARENB),So No Parity   */
    serial.c_cflag &= ~CSTOPB; /* CSTOPB = 2 Stop bits,here it is cleared so 1 Stop bit */
    serial.c_cflag &= ~CSIZE;  /* Clears the mask for setting the data size             */
    serial.c_cflag |= CS8;     /* Set the data bits = 8                                 */

    serial.c_cflag &= ~CRTSCTS;       /* No Hardware flow Control                         */
    serial.c_cflag |= CREAD | CLOCAL; /* Enable receiver,Ignore Modem Control lines       */

    serial.c_iflag &= ~(IXON | IXOFF | IXANY);         /* Disable XON/XOFF flow control both i/p and o/p */
    serial.c_iflag &= ~(ICANON | ECHO | ECHOE | ISIG); /* Non Cannonical mode                            */

    serial.c_oflag &= ~OPOST; /*No Output Processing*/

    /* Setting Time outs */
    serial.c_cc[VMIN] = 10; /* Read at least 10 characters */
    serial.c_cc[VTIME] = 0; /* Wait indefinetly   */
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

    // //attempt to send
    // if (write(fd, data, size) < 0)
    // {
    //     std::cout << "Failed to write packet" << std::endl;
    //     return;
    // }
}
