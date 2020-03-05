#pragma once

struct CommandInfo;

struct Command
{
  const char *name;
};

const Command *GetCommandFromData(const CommandInfo *data);