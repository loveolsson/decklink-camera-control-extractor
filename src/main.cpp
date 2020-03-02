#include "decklink.h"

#include <chrono>
#include <thread>
#include <signal.h>

static volatile int keepRunning = 1;

void intHandler(int dummy) {
    keepRunning = 0;
}

int main() {
	while (keepRunning) { 
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	
	return 0;
}
