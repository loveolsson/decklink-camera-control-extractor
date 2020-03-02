#include "decklink.h"

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

	while (keepRunning) { 
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	std::cout << "Exiting...\n";
	
	return 0;
}
