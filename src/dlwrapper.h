#pragma once
#include "include/DeckLinkAPI.h"

#include <iostream>
#include <cxxabi.h>
#include <string>

#define IID_FROM_TYPE(x) IID_##x
#define WRAPPED_FROM_IUNKNOWN(D, T) DLWrapper<T>::FromIUnknown(D, IID_FROM_TYPE(T))
#define WRAPPED_PRINT_FROM_IUNKNOWN(D, T) DLWrapper<T, true>::FromIUnknown(D, IID_FROM_TYPE(T))
#ifdef DISABLE_LOGGING
static const bool enableLogging = false;
#else
static const bool enableLogging = true;
#endif

template <typename T>
static std::string Demangle()
{
    // Returns an demangled version of the type name, or the mangled version if that call fails.
    std::string res = typeid(T).name();

    int status;
    char *c_str = abi::__cxa_demangle(res.c_str(), nullptr, nullptr, &status);

    if (status == 0)
    {
        res = c_str;
    }

    if (c_str != nullptr)
    {
        free(c_str);
    }

    return res;
}

template <typename T, bool Print = false>
class DLWrapper
{
public:
    DLWrapper()
        : DLWrapper(nullptr)
    {
    }

    // This is templated to accept to be set from a Wrapper with different PrintRelease
    template <bool P>
    DLWrapper(const DLWrapper<T, P> &_o)
        : item(_o.Get())
    {
        if (this->item)
        {
            std::cout << Demangle<T>() << " copied." << std::endl;
            this->item->AddRef();
        }
    }

    template <bool P>
    DLWrapper(DLWrapper<T, P> &&_o)
        : item(_o.Detach())
    {
        if (this->item)
        {
            std::cout << Demangle<T>() << " moved." << std::endl;
        }
    }

    explicit DLWrapper(T *_item)
        : item(_item)
    {
        if (this->item && (true || (Print && enableLogging)))
        {
            std::cout << "Wrapping: " << Demangle<T>() << " " << (uint64_t)this->item << std::endl;
        }
    }

    ~DLWrapper()
    {
        if (this->item)
        {
            if (true || (Print && enableLogging))
            {
                std::cout << "Releasing: " << Demangle<T>() << " " << (uint64_t)this->item << std::endl;
            }

            this->item->Release();
        }
    }

    explicit operator bool() const
    {
        return this->item != nullptr;
    }

    T *Get() const
    {
        return this->item;
    }

    T *Detach()
    {
        if (this->item) {
            std::cout << "Detach: " << Demangle<T>() << " " << (uint64_t)this->item << std::endl;
        }

        auto temp = this->item;
        this->item = nullptr;
        return temp;
    }

    T &operator*() const throw()
    {
        return *this->item;
    }

    T *operator->() const throw()
    {
        return this->item;
    }

    // For use in conjunction with the WRAPPED_FROM_IUNKNOWN macro to spin up wrapped instances from a IUnknown query
    static DLWrapper<T, Print> FromIUnknown(IUnknown *iface, REFIID iid)
    {
        if (iface == nullptr)
        {
            return DLWrapper<T, Print>();
        }

        T *t;
        if (SUCCEEDED(iface->QueryInterface(iid, (void **)&t)))
        {
            std::cout << "FromIUnknown(IUnknown): " << Demangle<T>() << " " << (uint64_t)this->item << std::endl;

            return DLWrapper<T, Print>(t);
        }

        return DLWrapper<T, Print>();
    }

    // For use in conjunction with the WRAPPED_FROM_IUNKNOWN macro to spin up wrapped instances from a wrapped IUnknown
    template <typename Q, bool P>
    static DLWrapper<T, Print> FromIUnknown(DLWrapper<Q, P> &wIface, REFIID iid)
    {
        std::cout << "FromIUnknown(DLWrapper<IUnknown>): " << Demangle<T>() << " " << (uint64_t)this->item << std::endl;
        return DLWrapper<T, Print>::FromIUnknown(wIface.Get(), iid);
    }

private:
    T *item;
};
