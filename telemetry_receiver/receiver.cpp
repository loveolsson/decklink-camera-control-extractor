#include "receiver.h"
#include "include/commands.h"
#include "uart.h"
#include "I2CCustom.h"

#include <memory>

#define RED_TALLY_EXCLUSIVE 1

// Takes the native format of 2 cameras being baked into one byte, and turns it into one byte per camera like the shield expects
static size_t BakeTallyData(const uint8_t in[], size_t inLen, uint8_t out[], size_t outLen)
{
  uint8_t *outPtr = out;

  if (outLen < (inLen - 1) * 2)
  {
    return 0;
  }

  for (int i = 1; i < inLen; ++i)
  {
    const uint8_t c1 = in[i] & 0x0F;
    const uint8_t c2 = in[i] >> 4;
    *outPtr++ = c1;
    *outPtr++ = c2;
  }

  return outPtr - out;
}

static void UpdateOptoTally(uint8_t state)
{
  bool r = state & 0x01;
  bool g = state & 0x02;

#ifdef RED_TALLY_EXCLUSIVE
  if (r)
  {
    g = false;
  }
#endif

  gpio_set_level(OPTO_O1, r ? 0 : 1); // Active low
  gpio_set_level(OPTO_O2, g ? 0 : 1);
}

bool IsTallyPacket(const Packet *pkt)
{
  return pkt->header.dest == 0xFF && pkt->commandInfo.category == 0xFF && pkt->commandInfo.parameter == 0xFF;
}

void ReceiverInit(RunState *runState)
{
  printf("Setting RS422 to read\n");
  gpio_set_level(RS422_RE, 0);

  InitUART();

  printf("Init shield...");
  runState->cc = std::make_shared<I2CCustom<BMD::SDICameraControl>>(SHIELD_ADDR);
  runState->cc->begin();
  runState->tally = std::make_shared<I2CCustom<BMD::SDITallyControl>>(SHIELD_ADDR);
  runState->tally->begin();
  printf("done\n");
  runState->cc->setOverride(true);
  runState->tally->setOverride(true);
  printf("ReceiverInit finished\n");
}

void ReceiverWireLoop(void *_runState)
{
  RunState *runState = static_cast<RunState *>(_runState);
  uint8_t data[255];
  uint8_t *dataPtr = data;

  Header peekHeader;

  while (true)
  {
    // Check if the shield is able to accept new data
    if (runState->cc->availableForWrite())
    {
      const size_t bytesInBuffer = dataPtr - data;

      // Check if any data has been accumulated
      if (bytesInBuffer > 0)
      {
        // Write data to shield.
        runState->cc->write(data, bytesInBuffer);

        // Reset ptr to start of data
        dataPtr = data;
      }
    }

    // Peek the header of the next packet in the fifo
    if (runState->queue.TPeek(&peekHeader) > 0)
    {
      const size_t bytesInBuffer = dataPtr - data;
      const size_t paddedPacketLength = PADDING(peekHeader.len);
      const size_t totalPacketLength = paddedPacketLength + sizeof(Header);

      // Check if packet fits in the buffer
      if (totalPacketLength < NUM(data) - bytesInBuffer)
      {
        // Set up a pointer to the data in place in the buffer
        const Packet *pkt = (Packet *)dataPtr;

        // Pop the entire packet from the fifo
        runState->queue.TPop(pkt, totalPacketLength);

        if (IsTallyPacket(pkt))
        {
          // This is a tally packet and needs to be handled separately and then discarded
          uint8_t tallyData[128];
          const size_t justDataSize = pkt->header.len - sizeof(CommandInfo);

          // Transform the tally data for the shield, returns the size of data in the output buffer
          const size_t tallyLength = BakeTallyData(pkt->data, justDataSize, tallyData, NUM(tallyData));

          if (tallyLength > 0)
          {
            // Read back camera ID from the DIP switches
            uint8_t dips = ReadDips();
            if (dips < tallyLength)
            {
              UpdateOptoTally(tallyData[dips]);
            }

            // Wait for the shield. This should only happen if there has been congestion on the RS422 line and
            // multiple tally packets are received within a frame boundary.
            while (!runState->tally->availableForWrite())
            {
              vTaskDelay(1);
            }

            // Write the data to the shield
            runState->tally->write(tallyData, tallyLength);
          }
        }
        else
        {
          // This is a regular packet so we forward the ptr;
          dataPtr += totalPacketLength;
        }
      }
    }

    vTaskDelay(1);
  }
}

struct MasterHeader
{
  uint8_t size;
  uint8_t crc;
};

void ReceiverSerialLoop(void *_runState)
{
  RunState *runState = static_cast<RunState *>(_runState);

  MasterHeader masterHeader;

  while (true)
  {
    // Search for the lead in bytes of 3 x 0xFE
    int counter = 0;
    while (counter < 3)
    {
      if (!UARTAvailable())
      {
        vTaskDelay(1);
        continue;
      }
      uint8_t c = UARTReadOneByte();

      if (c != 0xFE)
      {
        counter = 0;
      }
      else
      {
        counter++;
      }
    }

    // Wait for the master header to be available from the UART buffer
    while (UARTAvailable() < sizeof(MasterHeader))
    {
      vTaskDelay(1);
    }

    // Read the master header from UART
    UARTReadBytes((uint8_t *)&masterHeader, sizeof(MasterHeader));

    // Zero the packet struct to make sure padded bytes are zero (if any equipment would care)
    Packet pkt = {};

    // Check if the size of the packet exceeds maximum packet length
    if (masterHeader.size > sizeof(Packet))
    {
      continue;
    }

    if (masterHeader.size > 0)
    {
      // Wait for the full packet to be available from the UART buffer
      while (UARTAvailable() < masterHeader.size)
      {
        vTaskDelay(1);
      }

      // Read the full packet from UART
      UARTReadBytes((uint8_t *)&pkt, masterHeader.size);
    }

    // Check if the master header size matches the packet header size
    if (masterHeader.size != sizeof(Header) + pkt.header.len)
    {
      continue;
    }

    // Check if the master header crc matches the data
    if (masterHeader.crc != CRC((uint8_t *)&pkt, masterHeader.size))
    {
      continue;
    }

    // Push the packet to the fifo with padded size to make sure shield loop can pull the
    // full packet directly from the fifo.
    const size_t paddedSize = sizeof(Header) + PADDING(pkt.header.len);
    if (runState->queue.TPush(&pkt, paddedSize) == 0)
    {
      printf("FIFO full, failed to insert size %u\n", paddedSize);
      gpio_set_level(LED_ERR, 0);
    }
  }
}
