#include "decklink.h"

#include "defines.h"
#include "dlwrapper.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <ratio>
#include <string>

static constexpr auto tallyInterval = std::chrono::seconds(5);

std::shared_ptr<DLWrapper<IDeckLinkInput>>
GetDeckLinkByNameOrFirst(const std::string &name)
{
    auto wIterator = DLMakeShared(CreateDeckLinkIteratorInstance());

    if (!wIterator) {
        std::cout << "Could not initialize DeckLink driver." << std::endl;
        return nullptr;
    }

    IDeckLink *deckLink;

    while (SUCCEEDED((*wIterator)->Next(&deckLink))) {
        DLWrapper wDeckLink(deckLink);
        DLString devName;

        if (FAILED(deckLink->GetDisplayName(&devName))) {
            continue;
        }

        if (name.empty() || GetString(devName) == name) {
            auto wDeckLinkInput = WRAPPED_FROM_IUNKNOWN(deckLink, IDeckLinkInput);
            if (wDeckLinkInput) {
                return wDeckLinkInput;
            }
        }
    }

    return nullptr;
}

static Packet
BakeTallyPacket(const std::vector<uint8_t> &data)
{
    Packet pkt = {};

    pkt.header.dest    = 0xFF;
    pkt.header.command = 0xFF;
    pkt.header.len     = sizeof(CommandInfo) + data.size();

    pkt.commandInfo.category  = 0xFF;
    pkt.commandInfo.parameter = 0xFF;

    std::copy(data.begin(), data.end(), pkt.data);

    return pkt;
}

DeckLinkReceiver::DeckLinkReceiver(std::shared_ptr<DLWrapper<IDeckLinkInput>> _wDeckLinkInput,
                                   ByteFifo &_fifo)
    : wDeckLinkInput(_wDeckLinkInput)
    , fifo(_fifo)
    , lastTallyUpdate(std::chrono::steady_clock::now())
{
    if (!this->wDeckLinkInput) {
        std::cout << "DeckLinkReceiver created without ." << std::endl;
        return;
    }

    auto deckLinkInput = (*this->wDeckLinkInput).Get();

    {
        // Some legacy DeckLink cards can only capture VANC-data if the pixel format is 10bit
        auto attr = WRAPPED_FROM_IUNKNOWN(deckLinkInput, IDeckLinkProfileAttributes);
        if (!attr) {
            std::cout << "Could not get IDeckLinkProfileAttributes." << std::endl;
            return;
        }

        (*attr)->GetFlag(BMDDeckLinkVANCRequires10BitYUVVideoFrames, &this->requires10bit);

        if (this->requires10bit) {
            std::cout << "Requires 10bit for VANC." << std::endl;
        }
    }

    if (FAILED(deckLinkInput->SetCallback(this))) {
        std::cout << "Failed to set input callback." << std::endl;
        return;
    }

    BMDPixelFormat format = requires10bit ? bmdFormat10BitYUV : bmdFormat8BitYUV;

    if (FAILED(deckLinkInput->EnableVideoInput(bmdModeNTSC, format,
                                               bmdVideoInputEnableFormatDetection))) {
        std::cout << "Failed to enable video stream." << std::endl;
        return;
    }

    if (FAILED(deckLinkInput->StartStreams())) {
        std::cout << "Failed to start video stream." << std::endl;
    }
}

DeckLinkReceiver::~DeckLinkReceiver()
{
    if (this->wDeckLinkInput) {
        auto deckLinkInput = (*this->wDeckLinkInput).Get();

        std::cout << "Stopping input." << std::endl;
        deckLinkInput->StopStreams();
        deckLinkInput->FlushStreams();
    }
}

HRESULT
DeckLinkReceiver::VideoInputFrameArrived(IDeckLinkVideoInputFrame *videoFrame,
                                         IDeckLinkAudioInputPacket *)
{
    auto wPackets = WRAPPED_FROM_IUNKNOWN(videoFrame, IDeckLinkVideoFrameAncillaryPackets);
    if (!wPackets) {
        return S_OK;
    }

    auto packets = (*wPackets).Get();

    IDeckLinkAncillaryPacket *packet;
    const uint8_t *data;
    uint32_t size;

    // Check for tally data
    if (SUCCEEDED(packets->GetFirstPacketByID('Q', 'R', &packet))) {
        DLWrapper wrapped(packet);

        if (SUCCEEDED(
                packet->GetBytes(bmdAncillaryPacketFormatUInt8, (const void **)&data, &size))) {
            std::vector<uint8_t> newTallyData(data, data + size);

            const auto now             = std::chrono::steady_clock::now();
            const bool tallyHasChanged = this->activeTallyData != newTallyData;
            const bool doTimedUpdate   = now > (this->lastTallyUpdate + tallyInterval);

            if (tallyHasChanged) {
                this->activeTallyData.swap(newTallyData);
            }

            // Send tally data every [tallyInterval], or if the data has changed
            if (tallyHasChanged || doTimedUpdate) {
                // To send tally data we create a fake packet conforming to the standard that we can
                // strip on the receiving end

                // Check if data fits in package (this should be safe up to a 114 input mixer)
                if (this->activeTallyData.size() <= sizeof(Packet::data)) {
                    const Packet pkt = BakeTallyPacket(this->activeTallyData);
                    this->fifo.TPush(&pkt, pkt.header.GetPaddedTotalSize());
                }

                this->lastTallyUpdate = now;
            }
        }
    }

    // Check for camera control data
    if (SUCCEEDED(packets->GetFirstPacketByID('Q', 'S', &packet))) {
        DLWrapper wrapped(packet);

        if (SUCCEEDED(
                packet->GetBytes(bmdAncillaryPacketFormatUInt8, (const void **)&data, &size))) {
            this->fifo.Push(data, size);
        }
    }

    return S_OK;
}

HRESULT
DeckLinkReceiver::VideoInputFormatChanged(BMDVideoInputFormatChangedEvents notificationEvents,
                                          IDeckLinkDisplayMode *newDisplayMode,
                                          BMDDetectedVideoInputFormatFlags detectedSignalFlags)
{
    if (newDisplayMode) {
        DLString name;

        if (FAILED(newDisplayMode->GetName(&name))) {
            std::cout << "Couldn't get name from autodetected video mode" << std::endl;
            return S_OK;
        }

        std::cout << "Detected new video mode: " << GetString(name) << std::endl;

        auto deckLinkInput          = (*this->wDeckLinkInput).Get();
        const BMDPixelFormat format = this->requires10bit ? bmdFormat10BitYUV : bmdFormat8BitYUV;

        // VANC decoding doesn't start if the format is autodetected, so input needs to be
        // restarted with the correct mode
        deckLinkInput->StopStreams();
        deckLinkInput->DisableVideoInput();
        deckLinkInput->EnableVideoInput(newDisplayMode->GetDisplayMode(), format,
                                        bmdVideoInputEnableFormatDetection);
        deckLinkInput->StartStreams();
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
