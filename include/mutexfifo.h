#pragma once
#include <mutex>
#include <type_traits>

template <typename T, size_t S>
class MutexFifo
{
public:
  size_t Push(const T *data, size_t count)
  {
    std::unique_lock<std::mutex> lock(xMutex);
    if (this->itemCount + count > S)
    {
      return 0;
    }

    auto cb = [](T *fifoPtr, const T *userPtr, size_t size) {
      std::copy(userPtr, userPtr + size, fifoPtr);
    };

    this->writeHead = this->BatchCopy<const T>(data, this->writeHead, count, cb);
    this->itemCount += count;

    return count;
  }

  size_t Pop(T *data, size_t count)
  {
    std::unique_lock<std::mutex> lock(xMutex);

    if (count > this->itemCount)
    {
      return 0;
    }

    auto cb = [](T *fifoPtr, T *userPtr, size_t size) {
      std::copy(fifoPtr, fifoPtr + size, userPtr);
    };

    this->readHead = this->BatchCopy<T>(data, this->readHead, count, cb);
    this->itemCount -= count;

    return count;
  }

  size_t Peek(T *data, size_t count)
  {
    std::unique_lock<std::mutex> lock(xMutex);

    if (count > this->itemCount)
    {
      return 0;
    }

    auto cb = [](T *fifoPtr, T *userPtr, size_t size) {
      std::copy(fifoPtr, fifoPtr + size, userPtr);
    };

    this->BatchCopy<T>(data, this->readHead, count, cb);

    return count;
  }

  template <typename P>
  size_t TPush(const P *item, int size = -1)
  {
    if (size < 0)
    {
      size = sizeof(P);
    }

    return Push((T *)item, size);
  }

  template <typename P>
  size_t TPop(P *item, int size = -1)
  {
    if (size < 0)
    {
      size = sizeof(P);
    }

    return Pop((T *)item, size);
  }

  template <typename P>
  size_t TPeek(P *item, int size = -1)
  {
    if (size < 0)
    {
      size = sizeof(P);
    }

    return Peek((T *)item, size);
  }

private:
  // Call the callback one or two times to perform the transaction, depending on if the transaction
  // spans over the edge of the buffer. Returns the head position after copy.
  template <typename Tc>
  size_t BatchCopy(Tc *userPtr, size_t head, size_t count, void (*cb)(T *fifoPtr, Tc *userPtr, size_t size))
  {
    while (count > 0)
    {
      size_t itemsToCopy = S - head;
      if (count < itemsToCopy)
      {
        itemsToCopy = count;
      }

      cb(this->buffer + head, userPtr, itemsToCopy);

      count -= itemsToCopy;
      head += itemsToCopy;
      head %= S;
      userPtr += itemsToCopy;
    }

    return head;
  }

  std::mutex xMutex;
  T buffer[S];
  size_t itemCount = 0;
  size_t readHead = 0;
  size_t writeHead = 0;
};

using ByteFifo = MutexFifo<uint8_t, 2048>;