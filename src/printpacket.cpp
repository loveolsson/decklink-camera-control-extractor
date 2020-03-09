#include "printpacket.h"
#include "commands.h"
#include "defines.h"

#include <iomanip>
#include <sstream>

std::string PrintPacket(Packet &pkt)
{
    if (pkt.header.len < sizeof(CommandInfo))
    {
        return "Packet to short to decode";
    }

    std::stringstream str;

    str << "Dest: " << (int)pkt.header.dest;
    str << ", Len: " << (int)pkt.header.len;

    auto cmd = GetCommandFromData(&pkt.commandInfo);
    if (cmd != nullptr)
    {
        str << ", \"" << cmd->name << "\"";
    }
    else
    {
        str << ", \"Unknown command";
        str << " (" << (int)pkt.commandInfo.category;
        str << ", " << (int)pkt.commandInfo.parameter << ")";
    }

    if (pkt.commandInfo.type)
    {
        str << ", Type: assign";
    }
    else
    {
        str << ", Type: offset/toggle";
    }

    str << ", Hex:" << ToHex((uint8_t *)&pkt, sizeof(Header) + pkt.header.len);

    return str.str();
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