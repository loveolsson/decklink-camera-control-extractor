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

template <typename T, bool Print = false>
class DLWrapper
{
public:
    DLWrapper(T *_item)
        : item(_item)
    {
        if (this->item && Print && enableLogging)
        {
            std::string name = Demangle<T>();
            std::cout << "Wrapping: " << name << std::endl;
        }
    }

    DLWrapper()
        : DLWrapper(nullptr)
    {
    }

    // This is templated to accept to be set from a Wrapper with different PrintRelease
    template <bool P>
    DLWrapper(const DLWrapper<T, P> &_o)
        : DLWrapper(_o.Get())
    {
        if (this->item)
        {
            this->item->AddRef();
        }
    }

    ~DLWrapper()
    {
        if (this->item)
        {
            if (Print && enableLogging)
            {
                std::string name = Demangle<T>();
                std::cout << "Releasing: " << name << std::endl;
            }

            this->item->Release();
        }
    }

    explicit operator bool() const
    {
        return this->item != nullptr;
    }

    T *Get() const {
        return this->item;
    }

    T& operator*() const throw()
    {
        return *this->item;
    }

    T* operator->() const throw()
    {
        return this->item;
    }

    // For use in conjunction with the WRAPPED_FROM_IUNKNOWN macro to spin up wrapped instances from a IUnknown query
    static DLWrapper<T, Print> FromIUnknown(IUnknown *iface, REFIID iid)
    {
        if (iface == nullptr)
        {
            return nullptr;
        }

        T *t;
        if (iface->QueryInterface(iid, (void **)&t) == S_OK)
        {
            return t;
        }

        return nullptr;
    }

    // For use in conjunction with the WRAPPED_FROM_IUNKNOWN macro to spin up wrapped instances from a wrapped IUnknown
    template <typename Q, bool P>
    static DLWrapper<T, Print> FromIUnknown(DLWrapper<Q, P> &wIface, REFIID iid)
    {
        return DLWrapper<T, Print>::FromIUnknown(wIface.Get(), iid);
    }

private:
    T *item; 
};
