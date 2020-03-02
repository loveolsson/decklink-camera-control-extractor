#pragma once
#include "defines.h"

struct Command {
  const char* name;
};


const Command* GetCommandFromData(const CommandInfo* data);