#include "decklink.h"
#include "mutexfifo.h"
#include "defines.h"
#include "commands.h"
#include "printpacket.h"

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
			std::this_thread::sleep_for(std::chrono::microseconds(100));
			continue;
		}

		while (pkt.header.len > 0 && fifo.Pop((uint8_t *)&pkt.commandInfo, PADDING(pkt.header.len)) == 0)
		{
			std::this_thread::sleep_for(std::chrono::microseconds(100));
		}

		if (false)
		{
			//Serial2.write(leadIn, NUM(leadIn));
			//Serial2.write((byte *)&pkt, sizeof(Header) + pkt.header.len);
		}
		else if (true || pkt.header.dest == 1)
		{
			PrintPacket(pkt);
		}

	}

	std::cout << std::endl
			  << "Exiting..." << std::endl;

	return 0;
}
