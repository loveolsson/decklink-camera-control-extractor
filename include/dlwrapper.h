#pragma once
#include "include/DeckLinkAPI.h"

#include <iostream>
#include <memory>
#include <string>

#ifdef DLWRAPPER_LOGGING
#include <cxxabi.h>
#endif

#ifdef MAC
#include "CoreFoundation/CFBase.h"
#include "CoreFoundation/CFString.h"
#endif

#define IID_FROM_TYPE(x) IID_##x
#define WRAPPED_FROM_IUNKNOWN(D, T) DLWrapper<T>::FromIUnknown(D, IID_FROM_TYPE(T))
#ifdef DISABLE_LOGGING
static const bool enableLogging = false;
#else
static const bool enableLogging = true;
#endif

#ifdef MACOS
typedef CFStringRef DLString;
static std::string
GetString(DLString mstr)
{
    CFIndex length = CFStringGetLength(mstr);
    char *c_str    = (char *)malloc(length + 1);
    CFStringGetCString(mstr, c_str, length, kCFStringEncodingUTF8);

    std::string str(c_str);
    free(c_str);

    return str;
}
#else
typedef const char *DLString;
static std::string
GetString(DLString str)
{
    return str;
}
#endif

#ifdef DLWRAPPER_LOGGING
template <typename T>
static std::string
Demangle()
{
    static std::string res;
    if (!res.empty()) {
        return res;
    }

    // Returns an demangled version of the type name, or the mangled version if that call fails.
    res = typeid(T).name();

    int status;
    char *c_str = abi::__cxa_demangle(res.c_str(), nullptr, nullptr, &status);

    if (status == 0) {
        res = c_str;
    }

    if (c_str != nullptr) {
        free(c_str);
    }

    return res;
}
#endif

template <typename T>
class DLWrapper;

template <typename T>
static std::shared_ptr<DLWrapper<T>>
DLMakeShared(T *_item)
{
    if (!_item) {
        return nullptr;
    }

    return std::make_shared<DLWrapper<T>>(_item);
}

template <typename T>
class DLWrapper
{
public:
    explicit DLWrapper(T *_item)
        : item(_item)
    {
#ifdef DLWRAPPER_LOGGING
        std::cout << "Wrapping: " << Demangle<T>() << " " << (uint64_t)this->item << std::endl;
#endif
    }

    ~DLWrapper()
    {
#ifdef DLWRAPPER_LOGGING
        std::cout << "Releasing: " << Demangle<T>() << " " << (uint64_t)this->item << std::endl;
#endif
        if (this->item) {
            this->item->Release();
        }
    }

    T *
    Get() const
    {
        return this->item;
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
    static std::shared_ptr<DLWrapper<T>>
    FromIUnknown(IUnknown *iface, REFIID iid)
    {
        if (iface == nullptr) {
            return nullptr;
        }

        T *t;
        if (SUCCEEDED(iface->QueryInterface(iid, (void **)&t))) {
            return DLMakeShared(t);
        }

        return nullptr;
    }

    // For use in conjunction with the WRAPPED_FROM_IUNKNOWN macro to spin up wrapped instances from a wrapped IUnknown
    template <typename P>
    static std::shared_ptr<DLWrapper<T>>
    FromIUnknown(std::shared_ptr<DLWrapper<P>> wIface, REFIID iid)
    {
        if (!wIface) {
            return nullptr;
        }

        return FromIUnknown(&**wIface, iid);
    }

private:
    T *item;
};
