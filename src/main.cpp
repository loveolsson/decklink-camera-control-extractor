#include "decklink.h"
#include "mutexfifo.h"
#include "defines.h"
#include "commands.h"

#include <chrono>
#include <thread>
#include <signal.h>
#include <iostream>

static volatile int keepRunning = 1;
static const uint8_t leadIn[] = {0, 0, 0};


void intHandler(int)
{
	keepRunning = 0;
}

int main()
{
	signal(SIGINT, intHandler);

	ByteFifo fifo;

	auto threadId = std::this_thread::get_id();
	std::cout << "ThreadId: " << threadId << std::endl;
	std::cout << "Searching for DeckLink cards..." << std::endl;
	auto deckLink = GetFirstDeckLink();
	if (deckLink == nullptr)
	{
		std::cout << "Found no DeckLink cards... exiting." << std::endl;
		return 0;
	}

	DeckLinkReceiver receiver(deckLink, &fifo);
	Packet pkt;


	while (keepRunning)
	{
		if (fifo.Pop((uint8_t *)&pkt, sizeof(Header)) == 0)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}

		while (pkt.header.len > 0 && fifo.Pop((uint8_t *)&pkt.commandInfo, PADDING(pkt.header.len)) == 0)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		if (false)
		{
			//Serial2.write(leadIn, NUM(leadIn));
			//Serial2.write((byte *)&pkt, sizeof(Header) + pkt.header.len);
		}
		else if (true || pkt.header.dest == 1)
		{
			if (pkt.header.len < sizeof(CommandInfo))
			{
				continue;
			}

			printf("Dest: %i, Len: %i, ", pkt.header.dest, pkt.header.len);

			auto cmd = GetCommandFromData(&pkt.commandInfo);
			if (cmd != nullptr)
			{
				printf("\"%s\", Hex: ", cmd->name);
			}
			else
			{
				printf("\"Unknown command %i, %\", Hex:", pkt.commandInfo.category, pkt.commandInfo.parameter);
			}

			printf("Type: %s, ", pkt.commandInfo.type ? "assign" : "offset/toggle");

			for (int i = 0; i < sizeof(Header) + pkt.header.len; ++i)
			{
				printf("%02x", ((uint8_t *)&pkt)[i]);
			}

			printf("\n");
		}

	}

	std::cout << std::endl
			  << "Exiting..." << std::endl;

	return 0;
}
