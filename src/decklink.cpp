#include "decklink.h"
#include <iostream>

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

        if (this->deckLinkInput->QueryInterface(IID_IDeckLinkProfileAttributes, (void**)&attr) != S_OK) {
            std::cout << "Could not get IDeckLinkProfileAttributes." << std::endl;
            return;
        }
        
        bool requires10bit;
        attr->GetFlag(BMDDeckLinkVANCRequires10BitYUVVideoFrames, &requires10bit);

        if (requires10bit) {
            std::cout << "Requires 10bit for VANC." << std::endl;
        }

        attr->Release();


        if (this->deckLinkInput->SetCallback(this) != S_OK) {
            std::cout << "Failed to set input callback." << std::endl;
            return;
        }

        if (this->deckLinkInput->EnableVideoInput(bmdModeNTSC, bmdFormat10BitYUV, bmdVideoInputEnableFormatDetection) != S_OK) {
            std::cout << "Failed to enable video stream." << std::endl;
            return;
        }

        if (this->deckLinkInput->StartStreams() != S_OK) {
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
    IDeckLinkVideoFrameAncillaryPackets* packets;
    IDeckLinkAncillaryPacketIterator* iterator;
    IDeckLinkAncillaryPacket *packet;

    std::cout << "-";

    if (videoFrame->QueryInterface(IID_IDeckLinkVideoFrameAncillaryPackets, (void **)&packets) == S_OK) {
            std::cout << "p";

        if (packets->GetPacketIterator(&iterator) == S_OK) {
                std::cout << "i";

            while (iterator->Next(&packet) == S_OK) {
                uint32_t lineNum = packet->GetLineNumber();
                uint8_t did = packet->GetDID();
                uint8_t sdid = packet->GetSDID();
                std::cout << "Line num: " << lineNum << ", DID: " << did << ", SDID: " << sdid << std::endl;
                
                packet->Release();
            }

            iterator->Release();
        }

        packets->Release();
    }

    return S_OK;
}

HRESULT
DeckLinkReceiver::VideoInputFormatChanged(BMDVideoInputFormatChangedEvents notificationEvents, IDeckLinkDisplayMode *newDisplayMode, BMDDetectedVideoInputFormatFlags detectedSignalFlags)
{
    if (newDisplayMode) {
        const char* name;

        if (newDisplayMode->GetName(&name) == S_OK) {
            std::cout << "Detected new video mode: " << name << std::endl;
        }
    }

    return S_OK;
}

HRESULT 
DeckLinkReceiver::QueryInterface(REFIID, void**) {
    return S_FALSE;    
}

ULONG 
DeckLinkReceiver::AddRef() {
    return 0;
}

ULONG 
DeckLinkReceiver::Release() 
{
    return 0;
}
