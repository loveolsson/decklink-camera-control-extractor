#pragma once
#include "runstate.h"

void ReceiverInit(RunState *);
void ReceiverWireLoop(void *);
void ReceiverSerialLoop(void *);
