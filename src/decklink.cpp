#include "decklink.h"
#include "defines.h"

#include <stdlib.h>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <string.h>
#include <ratio>

#ifdef MAC
#include "CoreFoundation/CFBase.h"
#include "CoreFoundation/CFString.h"
#endif

static constexpr auto tallyInterval = std::chrono::seconds(1);

#ifdef MACOS
typedef CFStringRef OSString;
static std::string GetString(OSString mstr)
{
    CFIndex length = CFStringGetLength(mstr);
    char *c_str = (char *)malloc(length + 1);
    CFStringGetCString(mstr, c_str, length, kCFStringEncodingUTF8);

    std::string str(c_str);

    free(c_str);

    return str;
}
#else
typedef const char *OSString;
static std::string GetString(OSString str)
{
    return str;
}
#endif

DLWrapper<IDeckLink, true> GetDeckLinkByNameOrFirst(const char *name)
{
    IDeckLink *deckLink;
    DLWrapper<IDeckLinkIterator, true> wIterator(CreateDeckLinkIteratorInstance());

    if (!wIterator)
    {
        std::cout << "Could not initialize DeckLink driver." << std::endl;
        return nullptr;
    }

    while (wIterator->Next(&deckLink) == S_OK)
    {
        DLWrapper wDeckLink(deckLink);
        OSString devName;
        if (wDeckLink->GetDisplayName(&devName) == S_OK)
        {
            if (GetString(devName) == name || strlen(name) == 0)
            {
                return wDeckLink;
            }
        }
    }

    return nullptr;
}

DeckLinkReceiver::DeckLinkReceiver(DLWrapper<IDeckLink> deckLink, ByteFifo &_fifo)
    : fifo(_fifo), lastTallyUpdate(std::chrono::steady_clock::now())
{
    this->wDeckLinkInput = WRAPPED_FROM_IUNKNOWN(deckLink, IDeckLinkInput);
    if (!this->wDeckLinkInput)
    {
        std::cout << "DeckLink card has no input" << std::endl;
        return;
    }

    {
        // Some legacy DeckLink cards can only capture VANC-data if the pixel format is 10bit
        auto attr = WRAPPED_PRINT_FROM_IUNKNOWN(this->wDeckLinkInput, IDeckLinkProfileAttributes);
        if (!attr)
        {
            std::cout << "Could not get IDeckLinkProfileAttributes." << std::endl;
            return;
        }

        attr->GetFlag(BMDDeckLinkVANCRequires10BitYUVVideoFrames, &this->requires10bit);

        if (this->requires10bit)
        {
            std::cout << "Requires 10bit for VANC." << std::endl;
        }
    }

    if (this->wDeckLinkInput->SetCallback(this) != S_OK)
    {
        std::cout << "Failed to set input callback." << std::endl;
        return;
    }

    BMDPixelFormat format = requires10bit ? bmdFormat10BitYUV : bmdFormat8BitYUV;

    if (auto res = this->wDeckLinkInput->EnableVideoInput(bmdModeNTSC, format, bmdVideoInputEnableFormatDetection) != S_OK)
    {
        std::cout << "Failed to enable video stream: " << res << std::endl;
        return;
    }

    if (auto res = this->wDeckLinkInput->StartStreams() != S_OK)
    {
        std::cout << "Failed to start video stream: " << res << std::endl;
    }
}

DeckLinkReceiver::~DeckLinkReceiver()
{
    if (this->wDeckLinkInput)
    {
        std::cout << "Stopping input." << std::endl;
        this->wDeckLinkInput->StopStreams();
        this->wDeckLinkInput->FlushStreams();
    }
}

HRESULT
DeckLinkReceiver::VideoInputFrameArrived(IDeckLinkVideoInputFrame *videoFrame, IDeckLinkAudioInputPacket *)
{
    auto packets = WRAPPED_FROM_IUNKNOWN(videoFrame, IDeckLinkVideoFrameAncillaryPackets);
    if (!packets)
    {
        return S_OK;
    }

    IDeckLinkAncillaryPacket *packet;
    const uint8_t *data;
    uint32_t size;

    // Check for tally data
    if (packets->GetFirstPacketByID('Q', 'R', &packet) == S_OK)
    {
        DLWrapper wPacket(packet);
        if (packet->GetBytes(bmdAncillaryPacketFormatUInt8, (const void **)&data, &size) == S_OK)
        {
            std::vector<uint8_t> newTallyData(data, data + size);
            const auto now = std::chrono::steady_clock::now();

            const bool tallyHasChanged = this->activeTallyData != newTallyData;
            if (tallyHasChanged)
            {
                this->activeTallyData.swap(newTallyData);
            }

            // Send tally data every [tallyInterval], or if the data has changed
            if (tallyHasChanged || this->lastTallyUpdate + tallyInterval < now)
            {
                // To send tally data we create a fake packet conforming to the standard that we can
                // strip on the receiving end

                // Check if data fits in package (this should be safe up to a 114 input mixer)
                if (this->activeTallyData.size() <= sizeof(Packet::data))
                {
                    Packet pkt = {0};

                    pkt.header.dest = 255;
                    pkt.header.command = 255;
                    pkt.header.len = sizeof(CommandInfo) + size;

                    pkt.commandInfo.category = 255;
                    pkt.commandInfo.parameter = 255;

                    std::copy(this->activeTallyData.begin(), this->activeTallyData.end(), pkt.data);

                    const size_t totalSize = sizeof(Header) + PADDING(pkt.header.len);
                    this->fifo.Push((uint8_t *)&pkt, totalSize);
                }
            }
        }
    }

    // Check for camera control data
    if (packets->GetFirstPacketByID('Q', 'S', &packet) == S_OK)
    {
        DLWrapper wPacket(packet);
        if (packet->GetBytes(bmdAncillaryPacketFormatUInt8, (const void **)&data, &size) == S_OK)
        {
            this->fifo.Push(data, size);
        }
    }

    return S_OK;
}

HRESULT
DeckLinkReceiver::VideoInputFormatChanged(BMDVideoInputFormatChangedEvents notificationEvents, IDeckLinkDisplayMode *newDisplayMode, BMDDetectedVideoInputFormatFlags detectedSignalFlags)
{
    if (newDisplayMode)
    {
        OSString name;

        if (newDisplayMode->GetName(&name) == S_OK)
        {
            std::cout << "Detected new video mode: " << GetString(name) << std::endl;
        }

        const BMDPixelFormat format = this->requires10bit ? bmdFormat10BitYUV : bmdFormat8BitYUV;

        // We need to do this dance, bacause the VANC decoding doesn't start if the format is autodetected
        this->wDeckLinkInput->StopStreams();
        this->wDeckLinkInput->DisableVideoInput();
        this->wDeckLinkInput->EnableVideoInput(newDisplayMode->GetDisplayMode(), format, bmdVideoInputEnableFormatDetection);
        this->wDeckLinkInput->StartStreams();
    }

    return S_OK;
}

HRESULT
DeckLinkReceiver::QueryInterface(REFIID, void **)
{
    return S_FALSE;
}

ULONG
DeckLinkReceiver::AddRef()
{
    return 0;
}

ULONG
DeckLinkReceiver::Release()
{
    return 0;
}
