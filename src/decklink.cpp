#include "decklink.h"
#include <iostream>

static constexpr auto tallyInterval = std::chrono::seconds(1);

IDeckLink *GetDeckLinkByNameOrFirst(std::string &name)
{
    IDeckLink *deckLink = nullptr;
    Wrapper<IDeckLinkIterator, true> wIterator(CreateDeckLinkIteratorInstance());

    if (!wIterator)
    {
        std::cout << "Could not initialize DeckLink driver." << std::endl;
        return nullptr;
    }

    while (wIterator.Get()->Next(&deckLink) == S_OK)
    {
        Wrapper<IDeckLink> wDeckLink(deckLink);
        const char *devName;
        if (deckLink->GetDisplayName(&devName) == S_OK)
        {
            if (name == devName || name.empty())
            {
                deckLink->AddRef();
                return deckLink;
            }
        }
    }

    return nullptr;
}

DeckLinkReceiver::DeckLinkReceiver(IDeckLink *deckLink, ByteFifo *_fifo)
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
        auto attr = WRAPPED_FROM_IUNKNOWN(this->wDeckLinkInput, IDeckLinkProfileAttributes);
        if (!attr)
        {
            std::cout << "Could not get IDeckLinkProfileAttributes." << std::endl;
            return;
        }

        attr.Get()->GetFlag(BMDDeckLinkVANCRequires10BitYUVVideoFrames, &this->requires10bit);

        if (this->requires10bit)
        {
            std::cout << "Requires 10bit for VANC." << std::endl;
        }
    }

    if (this->wDeckLinkInput.Get()->SetCallback(this) != S_OK)
    {
        std::cout << "Failed to set input callback." << std::endl;
        return;
    }

    BMDPixelFormat format = requires10bit ? bmdFormat10BitYUV : bmdFormat8BitYUV;

    if (this->wDeckLinkInput.Get()->EnableVideoInput(bmdModeNTSC, format, bmdVideoInputEnableFormatDetection) != S_OK)
    {
        std::cout << "Failed to enable video stream." << std::endl;
        return;
    }

    if (this->wDeckLinkInput.Get()->StartStreams() != S_OK)
    {
        std::cout << "Failed to start video stream." << std::endl;
    }
}

DeckLinkReceiver::~DeckLinkReceiver()
{
    if (this->wDeckLinkInput.Get())
    {
        std::cout << "Stopping input." << std::endl;
        this->wDeckLinkInput.Get()->StopStreams();
        this->wDeckLinkInput.Get()->FlushStreams();
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
    if (packets.Get()->GetFirstPacketByID('Q', 'R', &packet) == S_OK)
    {
        Wrapper<IDeckLinkAncillaryPacket> wPacket(packet);
        if (packet->GetBytes(bmdAncillaryPacketFormatUInt8, (const void **)&data, &size) == S_OK)
        {
            std::vector<uint8_t> newTallyData(data, data + size);
            auto now = std::chrono::steady_clock::now();

            bool tallyHasChanged = false;
            if (this->activeTallyData != newTallyData)
            {
                this->activeTallyData = newTallyData;
                tallyHasChanged = true;
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

                    this->fifo->Push((uint8_t *)&pkt, totalSize);
                }
            }
        }
    }

    // Check for camera control data
    if (packets.Get()->GetFirstPacketByID('Q', 'S', &packet) == S_OK)
    {
        Wrapper<IDeckLinkAncillaryPacket> wPacket(packet);
        if (packet->GetBytes(bmdAncillaryPacketFormatUInt8, (const void **)&data, &size) == S_OK)
        {
            this->fifo->Push(data, size);
        }
    }

    return S_OK;
}

HRESULT
DeckLinkReceiver::VideoInputFormatChanged(BMDVideoInputFormatChangedEvents notificationEvents, IDeckLinkDisplayMode *newDisplayMode, BMDDetectedVideoInputFormatFlags detectedSignalFlags)
{
    if (newDisplayMode)
    {
        const char *name;

        if (newDisplayMode->GetName(&name) == S_OK)
        {
            std::cout << "Detected new video mode: " << name << std::endl;
        }
        
        // We need to do this dance, bacause the VANC decoding doesn't start if the format is autodetected
        BMDPixelFormat format = this->requires10bit ? bmdFormat10BitYUV : bmdFormat8BitYUV;

        this->wDeckLinkInput.Get()->StopStreams();
        this->wDeckLinkInput.Get()->DisableVideoInput();
        this->wDeckLinkInput.Get()->EnableVideoInput(newDisplayMode->GetDisplayMode(), format, bmdVideoInputEnableFormatDetection);
        this->wDeckLinkInput.Get()->StartStreams();
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
