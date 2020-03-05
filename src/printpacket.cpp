#include "printpacket.h"
#include "commands.h"  // for GetCommandFromData, Command
#include "defines.h"   // for Packet, Header, CommandInfo

#include <iomanip>     // for operator<<, setw
#include <iostream>    // for operator<<, basic_ostream, cout, ostream, stri...
#include <string>      // for char_traits
#include <sstream>

void PrintPacket(Packet &pkt)
{
    if (pkt.header.len < sizeof(CommandInfo))
    {
        return;
    }

    std::cout << "Dest: " << (int)pkt.header.dest;
    std::cout << ", Len: " << (int)pkt.header.len;

    auto cmd = GetCommandFromData(&pkt.commandInfo);
    if (cmd != nullptr)
    {
        std::cout << ", \"" << cmd->name << "\"";
    }
    else
    {
        std::cout << ", \"Unknown command";
        std::cout << "(" << pkt.commandInfo.category;
        std::cout << ", " << pkt.commandInfo.parameter << ")";
    }

    if (pkt.commandInfo.type)
    {
        std::cout << ", Type: assign";
    }
    else
    {
        std::cout << ", Type: offset/toggle";
    }

    std::cout << ", Hex:" << ToHex((uint8_t *)&pkt, sizeof(Header) + pkt.header.len);
    std::cout << std::endl;
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