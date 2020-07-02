#pragma once
#include <algorithm>
#include <exception>
#include <mutex>

template <typename T>
struct FifoCopyFromUser {
    using UserP = const T;

    void
    operator()(T fifoPtr[], UserP userPtr[], size_t count) const noexcept
    {
        std::copy(userPtr, userPtr + count, fifoPtr);
    }
};

template <typename T>
struct FifoCopyToUser {
    using UserP = T;

    void
    operator()(const T fifoPtr[], UserP userPtr[], size_t count) const noexcept
    {
        std::copy(fifoPtr, fifoPtr + count, userPtr);
    }
};

template <typename T, size_t S>
struct MutexFifo {
    static_assert(std::is_pod<T>::value, "Template type of MutexFifo is not POD");

    size_t
    Push(const T data[], size_t count) noexcept
    {
        std::lock_guard<std::mutex> lock(xMutex);

        if (this->itemCount + count > S) {
            return 0;
        }

        this->writeHead = this->BatchCopy<FifoCopyFromUser<T>>(data, this->writeHead, count);
        this->itemCount += count;

        return count;
    }

    size_t
    Pop(T data[], size_t count) noexcept
    {
        std::lock_guard<std::mutex> lock(xMutex);

        if (count > this->itemCount) {
            return 0;
        }

        this->readHead = this->BatchCopy<FifoCopyToUser<T>>(data, this->readHead, count);
        this->itemCount -= count;

        return count;
    }

    size_t
    Peek(T data[], size_t count) noexcept
    {
        std::lock_guard<std::mutex> lock(xMutex);

        if (count > this->itemCount) {
            return 0;
        }

        this->BatchCopy<FifoCopyToUser<T>>(data, this->readHead, count);

        return count;
    }

    template <typename P>
    size_t
    TPush(const P item[], int size = -1) noexcept
    {
        static_assert(std::is_pod<P>::value, "TPush templated with non-POD type.");
        static_assert(sizeof(P) % sizeof(T) == 0,
                      "TPush sizeof(P) does does not divide cleanly into sizeof(T).");

        if (size < 0) {
            size = sizeof(P) / sizeof(T);
        }

        return Push((T *)item, size);
    }

    template <typename P>
    size_t
    TPop(P item[], int size = -1) noexcept
    {
        static_assert(std::is_pod<P>::value, "TPop templated with non-POD type.");
        static_assert(sizeof(P) % sizeof(T) == 0,
                      "TPop sizeof(P) does does not divide cleanly into sizeof(T).");

        if (size < 0) {
            size = sizeof(P) / sizeof(T);
        }

        return Pop((T *)item, size);
    }

    template <typename P>
    size_t
    TPeek(P item[], int size = -1) noexcept
    {
        static_assert(std::is_pod<P>::value, "TPeek templated with non-POD type.");
        static_assert(sizeof(P) % sizeof(T) == 0,
                      "TPeek sizeof(P) does does not divide cleanly into sizeof(T).");

        if (size < 0) {
            size = sizeof(P) / sizeof(T);
        }

        return Peek((T *)item, size);
    }

    template <typename P>
    P
    PopOne()
    {
        P t;

        if (this->TPop(t.get()) == 0) {
            throw std::underflow_error("Not enough data in Fifo");
        }

        return t;
    }

    template <typename P>
    P
    PeekOne()
    {
        P t;

        if (this->TPeek(t.get()) == 0) {
            throw std::underflow_error("Not enough data in Fifo");
        }

        return t;
    }

private:
    // Call the callback one or two times to perform the transaction, depending on if the transaction
    // spans over the edge of the buffer. Returns the head position after copy.
    template <typename Func>
    size_t
    BatchCopy(typename Func::UserP userPtr[], size_t head, size_t count) noexcept
    {
        while (count > 0) {
            size_t itemsToCopy = S - head;
            if (count < itemsToCopy) {
                itemsToCopy = count;
            }

            Func cb;
            cb(&this->buffer[head], userPtr, itemsToCopy);

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
    size_t readHead  = 0;
    size_t writeHead = 0;
};

using ByteFifo = MutexFifo<uint8_t, 2048>;