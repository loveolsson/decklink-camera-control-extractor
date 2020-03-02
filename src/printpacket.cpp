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

    printf("Dest: %i, Len: %i, ", pkt.header.dest, pkt.header.len);

    auto cmd = GetCommandFromData(&pkt.commandInfo);
    if (cmd != nullptr)
    {
        printf("\"%s\", Hex: ", cmd->name);
    }
    else
    {
        printf("\"Unknown command %i, %i\", Hex:", pkt.commandInfo.category, pkt.commandInfo.parameter);
    }

    printf("Type: %s, ", pkt.commandInfo.type ? "assign" : "offset/toggle");

    for (int i = 0; i < sizeof(Header) + pkt.header.len; ++i)
    {
        printf("%02x", ((uint8_t *)&pkt)[i]);
    }

    printf("\n");
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