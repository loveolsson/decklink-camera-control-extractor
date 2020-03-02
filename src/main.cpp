#include "decklink.h"
#include "mutexfifo.h"

#include <chrono>
#include <thread>
#include <signal.h>
#include <iostream>

static volatile int keepRunning = 1;

void intHandler(int) {
    keepRunning = 0;
}

int main() {
	signal(SIGINT, intHandler);

	ByteFifo fifo;

	auto threadId = std::this_thread::get_id();
   	std::cout << "ThreadId: " << threadId << std::endl;
   	std::cout << "Searching for DeckLink cards..." << std::endl;
	auto deckLink = GetFirstDeckLink();
	if (deckLink == nullptr) {
		std::cout << "Found no DeckLink cards... exiting." << std::endl;
		return 0;
	}

	DeckLinkReceiver receiver(deckLink, &fifo);

	while (keepRunning) { 
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	std::cout << std::endl << "Exiting..." << std::endl;
	
	return 0;
}
