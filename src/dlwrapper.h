#pragma once
#include "include/DeckLinkAPI.h"

#include <iostream>
#include <cxxabi.h>
#include <string>

#define IID_FROM_TYPE(x) IID_##x
#define WRAPPED_FROM_IUNKNOWN(D, T) Wrapper<T>::FromIUnknown(D, IID_FROM_TYPE(T))

template <typename T>
static std::string Demangle()
{
    // Returns an demangled version of the type name, or the mangled version if that call fails.
    std::string res = typeid(T).name();

    size_t size = 64;
    int status;
    char *out = (char *)malloc(size);

    char *c_str = abi::__cxa_demangle(res.c_str(), out, &size, &status);

    if (c_str != nullptr && status == 0)
    {
        res = c_str;
    }

    return res;
}

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
                static std::string name = Demangle<T>();
                std::cout << "Releasing: " << name << std::endl;
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
        if (iface == nullptr)
        {
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
