#include "printpacket.h"
#include "commands.h"

#include <iostream>
#include <sstream>
#include <iomanip>

void PrintPacket(Packet &pkt)
{
    if (pkt.header.len < sizeof(CommandInfo))
    {
        return;
    }

    std::cout << "Dest: " << (int)pkt.header.dest << ", Len: " << (int)pkt.header.len << ", ";

    auto cmd = GetCommandFromData(&pkt.commandInfo);
    if (cmd != nullptr)
    {
        std::cout << "\"" << cmd->name << "\", Type: ";
    }
    else
    {
        std::cout << "\"Unknown command " << pkt.commandInfo.category << ", " << pkt.commandInfo.parameter << """, Type: ";
    }

    if (pkt.commandInfo.type) {
        std::cout << "assign ";
    } else {
        std::cout << "offset/toggle ";
    }

    std::cout << "Hex:" << ToHex((uint8_t *)&pkt, sizeof(Header) + pkt.header.len) << std::endl;
}

std::string ToHex(const uint8_t *buffer, size_t size)
{
    std::stringstream str;
    str.setf(std::ios_base::hex, std::ios::basefield);
    str.setf(std::ios_base::uppercase);
    str.fill('0');

    for (size_t i = 0; i < size; ++i)
    {
        str << std::setw(2) << (unsigned short)(uint8_t)buffer[i];
    }
    return str.str();
}