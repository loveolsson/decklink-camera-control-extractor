#include "decklink.h"
#include <iostream>
#include <thread>
#include <iomanip>
#include <string>
#include <sstream>

#define MAX_WIDTH_VANC 1920

static void unpack_v210(uint16_t *dst, const uint8_t *src, int width)
{
    int i;
    for (i = 0; i < width * 2 / 3; i++)
    {
        *dst++ = src[0] + ((src[1] & 3) << 8);
        *dst++ = (src[1] >> 2) + ((src[2] & 15) << 6);
        *dst++ = (src[2] >> 4) + ((src[3] & 63) << 4);
        src += 4;
    }
}

std::string ToHex(const uint8_t *buffer, size_t size)
{
    std::stringstream str;
    str.setf(std::ios_base::hex, std::ios::basefield);
    //str.unsetf(std::ios::showbase);
    //str.setf(std::ios::showbase);
    str.setf(std::ios_base::uppercase);
    str.fill('0');
    //str.width(2);

    for (size_t i = 0; i < size; ++i)
    {
        //str << std::hex << std::setw(2) << (unsigned short)(unsigned char)buffer[i];
        //str << (unsigned short)(unsigned char)buffer[i];
        str << std::setw(2) << (unsigned short)(uint8_t)buffer[i];
    }
    return str.str();
}

IDeckLink *GetFirstDeckLink()
{
    IDeckLink *deckLink = nullptr;

    IDeckLinkIterator *deckLinkIterator = CreateDeckLinkIteratorInstance();

    if (deckLinkIterator->Next(&deckLink) != S_OK)
    {
        deckLink = nullptr;
    }

    deckLinkIterator->Release();

    return deckLink;
}

DeckLinkReceiver::DeckLinkReceiver(IDeckLink *_deckLink)
    : deckLink(_deckLink)
{
    IDeckLinkProfileAttributes *attr;

    if (this->deckLink->QueryInterface(IID_IDeckLinkInput, (void **)&this->deckLinkInput) == S_OK)
    {
        std::cout << "DeckLink card has input" << std::endl;

        if (this->deckLinkInput->QueryInterface(IID_IDeckLinkProfileAttributes, (void **)&attr) != S_OK)
        {
            std::cout << "Could not get IDeckLinkProfileAttributes." << std::endl;
            return;
        }

        attr->GetFlag(BMDDeckLinkVANCRequires10BitYUVVideoFrames, &this->requires10bit);

        if (this->requires10bit)
        {
            std::cout << "Requires 10bit for VANC." << std::endl;
        }

        attr->Release();

        if (this->deckLinkInput->SetCallback(this) != S_OK)
        {
            std::cout << "Failed to set input callback." << std::endl;
            return;
        }

        BMDPixelFormat format = requires10bit ? bmdFormat10BitYUV : bmdFormat8BitYUV;

        if (this->deckLinkInput->EnableVideoInput(bmdModeNTSC, format, bmdVideoInputEnableFormatDetection) != S_OK)
        {
            std::cout << "Failed to enable video stream." << std::endl;
            return;
        }

        // if (this->deckLinkInput->EnableAudioInput(bmdAudioSampleRate48kHz, bmdAudioSampleType16bitInteger, 2) != S_OK) {
        //     std::cout << "Failed to enable audio stream." << std::endl;
        //     return;
        // }

        if (this->deckLinkInput->StartStreams() != S_OK)
        {
            std::cout << "Failed to start video stream." << std::endl;
        }
    }
}

DeckLinkReceiver::~DeckLinkReceiver()
{
    if (this->deckLinkInput)
    {
        std::cout << "Stopping input." << std::endl;
        this->deckLinkInput->StopStreams();
        this->deckLinkInput->FlushStreams();

        std::cout << "Releasing input." << std::endl;
        this->deckLinkInput->Release();
    }

    if (this->deckLink)
    {
        std::cout << "Releasing card." << std::endl;
        this->deckLink->Release();
    }
}

HRESULT
DeckLinkReceiver::VideoInputFrameArrived(IDeckLinkVideoInputFrame *videoFrame, IDeckLinkAudioInputPacket *)
{
    IDeckLinkVideoFrameAncillary *anc;

    IDeckLinkVideoFrameAncillaryPackets *packets;
    IDeckLinkAncillaryPacket *packet;

    //auto threadId = std::this_thread::get_id();
    //std::cout << "Frame threadId: " << threadId << std::endl;

    if (videoFrame->QueryInterface(IID_IDeckLinkVideoFrameAncillaryPackets, (void **)&packets) == S_OK)
    {
        const uint8_t *data;
        uint32_t size;

        uint16_t vanc[MAX_WIDTH_VANC];
        size_t vanc_size = videoFrame->GetWidth();

        if (packets->GetFirstPacketByID('Q', 'R', &packet) == S_OK)
        {
            if (packet->GetBytes(bmdAncillaryPacketFormatUInt8, (const void **)&data, &size) == S_OK)
            {
                std::cout << "QS Len: " << size << " " << ToHex(data, size) << std::endl;
            }

            packet->Release();
        }

        if (packets->GetFirstPacketByID('Q', 'S', &packet) == S_OK)
        {
            if (packet->GetBytes(bmdAncillaryPacketFormatUInt8, (const void **)&data, &size) == S_OK)
            {
                std::cout << "QS Len: " << size << " " << ToHex(data, size) << std::endl;
            }
            packet->Release();
        }

        packets->Release();
    }

    std::cout << std::flush;

    return S_OK;
}

HRESULT
DeckLinkReceiver::VideoInputFormatChanged(BMDVideoInputFormatChangedEvents notificationEvents, IDeckLinkDisplayMode *newDisplayMode, BMDDetectedVideoInputFormatFlags detectedSignalFlags)
{
    auto threadId = std::this_thread::get_id();
    std::cout << "VideoInputFormatChanged threadId: " << threadId << std::endl;

    if (newDisplayMode)
    {
        const char *name;

        if (newDisplayMode->GetName(&name) == S_OK)
        {
            std::cout << "Detected new video mode: " << name << std::endl;
        }
    }

    BMDPixelFormat format = requires10bit ? bmdFormat10BitYUV : bmdFormat8BitYUV;
    this->deckLinkInput->StopStreams();
    this->deckLinkInput->DisableVideoInput();
    this->deckLinkInput->EnableVideoInput(newDisplayMode->GetDisplayMode(), format, bmdVideoInputEnableFormatDetection);
    this->deckLinkInput->StartStreams();

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
