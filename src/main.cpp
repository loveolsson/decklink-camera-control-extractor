#include "decklink.h"
#include "mutexfifo.h"
#include "printpacket.h"
#include "serialoutput.h"

#include <chrono>
#include <thread>
#include <signal.h>
#include <iostream>

static volatile int keepRunning = 1;

static void intHandler(int)
{
	keepRunning = 0;
}

static void PrintUsage()
{
	std::cout << "Usage:" << std::endl;
	std::cout << "\t./DeckLinkCameraControl /dev/ttyS0 19200 \"DeckLink device\"" << std::endl
			  << std::endl;
	std::cout << "\tSerial device (/dev/ttyS[] on Linux, /dev/cu.[] on Mac)." << std::endl;
	std::cout << "\tBaud rate [Optional], defaults to 19200." << std::endl;
	std::cout << "\tDeckLink device [Optional], defaults to first device." << std::endl
			  << std::endl;
}

int main(int argc, char *argv[])
{
	signal(SIGINT, intHandler);

	int baudRate = 19200;

	std::vector<std::string> args;
	for (int i = 1; i < argc; ++i)
	{
		args.push_back(argv[i]);
	}

	if (args.empty() || args.size() > 3)
	{
		PrintUsage();
		return EXIT_FAILURE;
	}

	if (args.size() > 1)
	{
		try
		{
			baudRate = std::stoi(args[1]);
		}
		catch (const std::exception &)
		{
			std::cout << "Could not parse baud rate. Exiting..." << std::endl;
			PrintUsage();
			return EXIT_FAILURE;
		}
	}

	SerialOutput serialOutput(args[0], baudRate);
	if (!serialOutput.Begin())
	{
		std::cout << "Failed to open serial device: " << args[0] << std::endl;
		PrintUsage();
		return EXIT_FAILURE;
	}

	std::cout << "Searching for DeckLink cards..." << std::endl;

	std::string deckLinkName = args.size() == 3 ? args[2] : "";
	DLWrapper<IDeckLink, true> wDeckLink(GetDeckLinkByNameOrFirst(deckLinkName));
	if (!wDeckLink)
	{
		std::cout << "Found no DeckLink cards... exiting." << std::endl;
		PrintUsage();
		return EXIT_FAILURE;
	}

	ByteFifo fifo;
	DeckLinkReceiver receiver(wDeckLink, fifo);
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
				return EXIT_FAILURE;
			}
		}

		if (!gotJobDone)
		{
			std::this_thread::sleep_for(std::chrono::microseconds(100));
		}
	}

	std::cout << std::endl
			  << "Exiting..." << std::endl;

	return EXIT_SUCCESS;
}
