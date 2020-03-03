#include "decklink.h"
#include "mutexfifo.h"
#include "defines.h"
#include "commands.h"
#include "printpacket.h"
#include "serialoutput.h"

#include <chrono>
#include <thread>
#include <signal.h>
#include <iostream>

static volatile int keepRunning = 1;

void intHandler(int)
{
	keepRunning = 0;
}

int main(int argc, char *argv[])
{
	std::string serialDevice;
	std::string deckLinkName;
	int baudRate = 19200;

	if (argc < 2 || argc > 4)
	{
		std::cout << "\tUsage: ./DeckLinkCameraControl /dev/ttyS0 19200 \"DeckLink display name\"" << std::endl;
		std::cout << "\tDefault baud rate is 19200. If DeckLink display name is omitted, the first DeckLink device will be selected." << std::endl;
		std::cout << "\tExiting..." << std::endl;
		return 0;
	}

	serialDevice = argv[1];

	if (argc > 2)
	{
		try
		{
			baudRate = std::stoi(argv[2]);
		}
		catch (const std::exception &)
		{
			std::cout << "Could not parse baud rate. Exiting..." << std::endl;
			return 0;
		}
	}

	if (argc == 4)
	{
		deckLinkName = argv[3];
	}
	else
	{
		std::cout << "No DeckLink device name specified, using first device." << std::endl;
	}

	signal(SIGINT, intHandler);

	SerialOutput serialOutput(serialDevice, baudRate);
	if (!serialOutput.Begin())
	{
		std::cout << "Failed to open serial device: " << serialDevice << std::endl;
		return 0;
	}

	ByteFifo fifo;

	std::cout << "Searching for DeckLink cards..." << std::endl;

	Wrapper<IDeckLink, true> wDeckLink(GetDeckLinkByNameOrFirst(deckLinkName));
	if (wDeckLink.Get() == nullptr)
	{
		std::cout << "Found no DeckLink cards... exiting." << std::endl;
		return 0;
	}

	DeckLinkReceiver receiver(wDeckLink.Get(), &fifo);
	Packet pkt;

	while (keepRunning)
	{
		bool gotJobDone = false;

		if (fifo.Peek((uint8_t *)&pkt.header, sizeof(Header)) != 0)
		{
			const size_t totalLength = sizeof(Header) + PADDING(pkt.header.len);

			if (fifo.Pop((uint8_t *)&pkt, totalLength) != 0)
			{
				gotJobDone = true;

				serialOutput.Write((uint8_t *)&pkt, totalLength);
				PrintPacket(pkt);
			}
			else
			{
				// If a header is written to the fifo, the full message should also be available
				std::cout << "Not enough data in fifo, exiting..." << std::endl;
				return 0;
			}
		}

		if (!gotJobDone)
		{
			std::this_thread::sleep_for(std::chrono::microseconds(100));
		}
	}

	std::cout << std::endl
			  << "Exiting..." << std::endl;

	return 0;
}
