#include "decklink.h"
#include "defines.h"
#include "dlwrapper.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <string.h>

static constexpr auto tallyInterval = std::chrono::seconds(5);

std::shared_ptr<DLWrapper<IDeckLinkInput>> GetDeckLinkByNameOrFirst(const char *name)
{
    IDeckLink *deckLink;
    auto wIterator = DLMakeShared(CreateDeckLinkIteratorInstance());

    if (!wIterator)
    {
        std::cout << "Could not initialize DeckLink driver." << std::endl;
        return nullptr;
    }

    while ((*wIterator)->Next(&deckLink) == S_OK)
    {
        auto wDeckLink = DLMakeShared(deckLink);
        DLString devName;
        if ((*wDeckLink)->GetDisplayName(&devName) == S_OK)
        {
            if (GetString(devName) == name || strlen(name) == 0)
            {
                auto wDeckLinkInput = WRAPPED_FROM_IUNKNOWN(wDeckLink, IDeckLinkInput);
                if (wDeckLinkInput)
                {
                    return wDeckLinkInput;
                }
            }
        }
    }

    return nullptr;
}

DeckLinkReceiver::DeckLinkReceiver(std::shared_ptr<DLWrapper<IDeckLinkInput>> _wDeckLinkInput, ByteFifo &_fifo)
    : wDeckLinkInput(_wDeckLinkInput), fifo(_fifo), lastTallyUpdate(std::chrono::steady_clock::now())
{
    {
        // Some legacy DeckLink cards can only capture VANC-data if the pixel format is 10bit
        auto attr = WRAPPED_FROM_IUNKNOWN(this->wDeckLinkInput, IDeckLinkProfileAttributes);
        if (!attr)
        {
            std::cout << "Could not get IDeckLinkProfileAttributes." << std::endl;
            return;
        }

        (*attr)->GetFlag(BMDDeckLinkVANCRequires10BitYUVVideoFrames, &this->requires10bit);

        if (this->requires10bit)
        {
            std::cout << "Requires 10bit for VANC." << std::endl;
        }
    }

    if (FAILED((*this->wDeckLinkInput)->SetCallback(this)))
    {
        std::cout << "Failed to set input callback." << std::endl;
        return;
    }

    BMDPixelFormat format = requires10bit ? bmdFormat10BitYUV : bmdFormat8BitYUV;

    if (FAILED((*this->wDeckLinkInput)->EnableVideoInput(bmdModeNTSC, format, bmdVideoInputEnableFormatDetection)))
    {
        std::cout << "Failed to enable video stream." << std::endl;
        return;
    }

    if (FAILED((*this->wDeckLinkInput)->StartStreams()))
    {
        std::cout << "Failed to start video stream." << std::endl;
    }
}

DeckLinkReceiver::~DeckLinkReceiver()
{
    if (this->wDeckLinkInput)
    {
        std::cout << "Stopping input." << std::endl;
        (*this->wDeckLinkInput)->StopStreams();
        (*this->wDeckLinkInput)->FlushStreams();
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
    if (SUCCEEDED((*packets)->GetFirstPacketByID('Q', 'R', &packet)))
    {
        auto wPacket = DLMakeShared(packet);
        if ((*wPacket)->GetBytes(bmdAncillaryPacketFormatUInt8, (const void **)&data, &size) == S_OK)
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
                this->lastTallyUpdate = now;
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
    if (SUCCEEDED((*packets)->GetFirstPacketByID('Q', 'S', &packet)))
    {
        auto wPacket = DLMakeShared(packet);

        if (SUCCEEDED((*wPacket)->GetBytes(bmdAncillaryPacketFormatUInt8, (const void **)&data, &size)))
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
        DLString name;
        if (newDisplayMode->GetName(&name) == S_OK)
        {
            std::cout << "Detected new video mode: " << GetString(name) << std::endl;
        }

        const BMDPixelFormat format = this->requires10bit ? bmdFormat10BitYUV : bmdFormat8BitYUV;

        // We need to do this dance, bacause the VANC decoding doesn't start if the format is autodetected
        (*this->wDeckLinkInput)->StopStreams();
        (*this->wDeckLinkInput)->DisableVideoInput();
        (*this->wDeckLinkInput)->EnableVideoInput(newDisplayMode->GetDisplayMode(), format, bmdVideoInputEnableFormatDetection);
        (*this->wDeckLinkInput)->StartStreams();
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
