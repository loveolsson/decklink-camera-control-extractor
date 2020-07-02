#include "decklink.h"
#include "defines.h"  // for Packet, Header
#include "mutexfifo.h"
#include "printpacket.h"
#include "serialoutput.h"

#include <stdlib.h>      // for EXIT_FAILURE, EXIT_SUCCESS, size_t
#include <sys/signal.h>  // for signal, SIGINT

#include <algorithm>
#include <chrono>
#include <exception>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

static volatile int keepRunning = 1;

static void
intHandler(int)
{
    keepRunning = 0;
}

static void
PrintUsage()
{
    std::cout << "Usage:" << std::endl;
    std::cout << "\t./DeckLinkCameraControl /dev/ttyS0 19200 1,2,3 \"DeckLink device\"" << std::endl
              << std::endl;
    std::cout << "\tSerial device (/dev/ttyS[] on Linux, /dev/cu.[] on Mac)." << std::endl;
    std::cout << "\tBaud rate [Optional], defaults to 19200." << std::endl;
    std::cout << "\tCamera list [Optional], defaults to 1." << std::endl;
    std::cout << "\tDeckLink device [Optional], defaults to first device." << std::endl
              << std::endl;
}

static void
SplitAndParseIntVector(std::vector<int> &res, const std::string &strToSplit)
{
    std::stringstream ss(strToSplit);
    std::string item;
    while (std::getline(ss, item, ',')) {
        res.push_back(std::stoi(item));
    }
}

int
main(int argc, char *argv[])
{
    signal(SIGINT, intHandler);
    int baudRate                     = 19200;
    const std::string argSerialDev   = argc >= 2 ? argv[1] : "";
    const std::string argSerialRate  = argc >= 3 ? argv[2] : "";
    const std::string argCameraList  = argc >= 4 ? argv[3] : "1";
    const std::string argDeckLinkDev = argc >= 5 ? argv[4] : "";
    std::vector<int> cameraList;

    if (argSerialDev.empty() || argc > 5) {
        PrintUsage();
        return EXIT_FAILURE;
    }

    if (!argSerialRate.empty()) {
        try {
            baudRate = std::stoi(argSerialRate);
        } catch (const std::exception &) {
            std::cout << "Could not parse baud rate. Exiting..." << std::endl;
            PrintUsage();
            return EXIT_FAILURE;
        }
    }

    try {
        SplitAndParseIntVector(cameraList, argCameraList);
    } catch (const std::exception &e) {
        std::cout << "Could not parse camera list. Exiting..." << std::endl;
        PrintUsage();
        return EXIT_FAILURE;
    }

    SerialOutput serialOutput(argSerialDev.c_str(), baudRate);
    if (!serialOutput.Begin()) {
        std::cout << "Failed to open serial device: " << argSerialDev << std::endl;
        PrintUsage();
        return EXIT_FAILURE;
    }

    std::cout << "Searching for DeckLink cards..." << std::endl;

    auto wDeckLinkInput = GetDeckLinkByNameOrFirst(argDeckLinkDev);
    if (!wDeckLinkInput) {
        std::cout << "Found no DeckLink cards... exiting." << std::endl;
        PrintUsage();
        return EXIT_FAILURE;
    }

    ByteFifo fifo;
    DeckLinkReceiver receiver(wDeckLinkInput, fifo);
    Packet pkt;

    while (keepRunning) {
        bool gotJobDone = false;

        if (fifo.TPop(&pkt.header) != 0) {
            if (fifo.TPop(&pkt.commandInfo, pkt.header.GetPaddedSize()) != 0) {
                gotJobDone = true;

                bool sendPkg = (pkt.header.dest == 255);  // Broadcast / Tally
                if (!sendPkg) {
                    auto it = std::find(cameraList.begin(), cameraList.end(), pkt.header.dest);
                    sendPkg = it != cameraList.end();
                }

                if (sendPkg) {
                    serialOutput.Write(pkt);
                    std::cout << PrintPacket(pkt) << std::endl;
                }
            } else {
                // If a header is written to the fifo, the full message should also be available
                std::cout << "Not enough data in fifo, exiting..." << std::endl;
                return EXIT_FAILURE;
            }
        }

        if (!gotJobDone) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }

    std::cout << std::endl << "Exiting..." << std::endl;

    return EXIT_SUCCESS;
}
