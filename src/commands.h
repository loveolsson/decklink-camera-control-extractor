#pragma once
#include "defines.h"

struct Command {
  const char* name;
};

struct CommandGroup {
  const Command* commands;
  const int commandCount;
};

const Command* GetCommandFromData(const CommandInfo* data);