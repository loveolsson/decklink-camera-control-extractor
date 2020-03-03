#pragma once
#include "include/LinuxCOM.h"
#include <cstdint>
#include <iostream>

#define NUM(a) (sizeof(a) / sizeof(*a))
#define PADDING(x) (((x + 3) / 4) * 4)
#define IID_FROM_TYPE(x) IID_##x
#define WRAPPED_FROM_IUNKNOWN(D, T) Wrapper<T>::FromIUnknown(D, IID_FROM_TYPE(T))

#pragma pack(push, 1)
struct Header
{
  uint8_t dest;
  uint8_t len;
  uint8_t command;
  uint8_t _reserved;
};

struct CommandInfo
{
  uint8_t category;
  uint8_t parameter;
  uint8_t type;
};

#define PACKET_DATA_LENGTH (64 - sizeof(Header) - sizeof(CommandInfo))

struct Packet
{
  Header header;
  CommandInfo commandInfo;
  uint8_t data[PACKET_DATA_LENGTH];
};
#pragma pack(pop)

template <typename T>
struct TypeName
{
  static const char *Get()
  {
    return typeid(T).name();
  }
};

template <typename T, bool PrintRelease = false>
class Wrapper
{
public:
  Wrapper()
      : item(nullptr)
  {
  }

  Wrapper(T *_item)
      : item(_item)
  {
  }

  template <bool P>
  Wrapper(const Wrapper<T, P> &_o)
      : item(_o.Get())
  {
    this->item->AddRef();
  }

  ~Wrapper()
  {
    if (item)
    {
      if (PrintRelease)
      {
        std::cout << "Releasing: " << TypeName<T>().Get() << std::endl;
      }
      item->Release();
    }
  }

  T *Get() const
  {
    return item;
  }

  explicit operator bool() const
  {
    return this->item != nullptr;
  }

  static Wrapper<T, PrintRelease> FromIUnknown(IUnknown *iface, REFIID idd)
  {
    if (iface == nullptr) {
      return nullptr;
    }

    T *t;
    if (iface->QueryInterface(idd, (void **)&t) == S_OK)
    {
      return t;
    }

    return nullptr;
  }

  template <typename Q, bool P>
  static Wrapper<T, PrintRelease> FromIUnknown(Wrapper<Q, P> &wIface, REFIID idd)
  {
    return Wrapper<T, PrintRelease>::FromIUnknown(wIface.Get(), idd);
  }

private:
  T *item;
};
