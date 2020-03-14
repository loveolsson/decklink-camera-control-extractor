#include "include/commands.h"
#include "runstate.h"

#include "esp32_defines.h"

#include <memory>

void
SenderInit(RunState *runState)
{
#if 0
  printf("Init...");
  runState->cco.begin();
  printf("done\n");

  if (runState->params.rs422)
  {
    WritePin(RS422_DE, true);
  }
#endif
}

void
SenderWireLoop(void *_runState)
{
#if 0
  RunState *runState = static_cast<RunState *>(_runState);
  byte data[256];

  while (true)
  {
    //memset(data, 0, NUM(data));
    int readSize = runState->cco.read(data, NUM(data));

    if (readSize < PADDING(1))
    {
      vTaskDelay(0);
      continue;
    }
    printf("Readsize: %i/PADDING(%i) \n", readSize, PADDING(readSize));

    runState->queue.Push(data, PADDING(readSize));
  }
#endif
}

//const PROGMEM byte leadIn[] = {0, 0, 0};

void
SenderSerialLoop(void *_runState)
{
#if 0
  RunState *runState = static_cast<RunState *>(_runState);

  Packet pkt;

  while (true)
  {
    if (runState->queue.Pop((byte *)&pkt, sizeof(Header)) == 0)
    {
      vTaskDelay(0);
      continue;
    }

    while (pkt.header.len > 0 && runState->queue.Pop((byte *)&pkt.commandInfo, PADDING(pkt.header.len)) == 0)
    {
      vTaskDelay(0);
    }

    if (runState->params.rs422)
    {
      //Serial2.write(leadIn, NUM(leadIn));
      //Serial2.write((byte*)&pkt, sizeof(Header) + pkt.header.len);
    }
    else if (true || pkt.header.dest == 1)
    {
      if (pkt.header.len < sizeof(CommandInfo))
      {
        continue;
      }

      printf("Dest: %i, Len: %i, ", pkt.header.dest, pkt.header.len);

      auto cmd = GetCommandFromData(&pkt.commandInfo);
      if (cmd != nullptr)
      {
        printf("\"%s\", Hex: ", cmd->name);
      }
      else
      {
        printf("\"Unknown command %i, %\", Hex:", pkt.commandInfo.category, pkt.commandInfo.parameter);
      }

      printf("Type: %s, ", pkt.commandInfo.type ? "assign" : "offset/toggle");

      for (int i = 0; i < sizeof(Header) + pkt.header.len; ++i)
      {
        printf("%02x", ((byte *)&pkt)[i]);
      }

      printf("\n");
    }
  }
#endif
}
