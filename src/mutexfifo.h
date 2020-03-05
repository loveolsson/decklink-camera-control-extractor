#pragma once
#include <mutex>
#include <algorithm>

template <typename T, size_t S>
class MutexFifo
{
public:
  size_t Push(const T *data, size_t count)
  {
    std::unique_lock<std::mutex> lock(xMutex);
    if (this->itemCount + count > S)
    {
      count = 0;
    }

    size_t itemsLeft = count;

    while (itemsLeft > 0)
    {
      const size_t leftBeforeWrap = S - this->writeHead;
      const size_t itemsToWrite = std::min(itemsLeft, leftBeforeWrap);

      std::copy(data, data + itemsToWrite, this->queue + this->writeHead);

      itemsLeft -= itemsToWrite;
      this->itemCount += itemsToWrite;
      this->writeHead += itemsToWrite;
      this->writeHead %= S;
      data = &data[itemsToWrite];
    }

    return count;
  }

  size_t Pop(T *data, size_t count)
  {
    std::unique_lock<std::mutex> lock(xMutex);

    if (count == 0 || count > this->itemCount)
    {
      return 0;
    }

    size_t itemsLeft = count;

    while (itemsLeft > 0)
    {
      const size_t leftBeforeWrap = S - this->readHead;
      const size_t itemsToRead = std::min(itemsLeft, leftBeforeWrap);

      const uint8_t* readStart = &this->queue[readHead];
      std::copy(readStart, readStart + itemsToRead, data);

      itemsLeft -= itemsToRead;
      this->itemCount -= itemsToRead;
      this->readHead += itemsToRead;
      this->readHead %= S;
      data = &data[itemsToRead];
    }

    return count;
  }

  size_t Peek(T *data, size_t count)
  {
    std::unique_lock<std::mutex> lock(xMutex);

    if (count < 0 || count > this->itemCount)
    {
      return 0;
    }

    size_t tempReadHead = this->readHead;
    size_t itemsLeft = count;

    while (itemsLeft > 0)
    {
      const size_t leftBeforeWrap = S - tempReadHead;
      const size_t itemsToRead = std::min(itemsLeft, leftBeforeWrap);

      const uint8_t* readStart = &this->queue[readHead];
      std::copy(readStart, readStart + itemsToRead, data);

      itemsLeft -= itemsToRead;
      tempReadHead = (tempReadHead + itemsToRead) % S;
      data = &data[itemsToRead];
    }

    return count;
  }

private:
  std::mutex xMutex;
  T queue[S];
  size_t itemCount = 0;
  size_t readHead = 0;
  size_t writeHead = 0;
};

using ByteFifo = MutexFifo<uint8_t, 2048>;