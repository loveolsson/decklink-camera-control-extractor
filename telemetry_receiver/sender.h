#pragma once
#include "runstate.h"

void SenderInit(RunState *);
void SenderWireLoop(void *);
void SenderSerialLoop(void *);
