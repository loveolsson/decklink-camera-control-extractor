#include "decklink.h"
#include <iostream>
#include <thread>

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

        BMDPixelFormat format = requires10bit ? bmdFormat10BitYUV : bmdFormat8BitYUV;

        if (this->deckLinkInput->EnableVideoInput(bmdModeNTSC, format, bmdVideoInputEnableFormatDetection) != S_OK) {
            std::cout << "Failed to enable video stream." << std::endl;
            return;
        }

        // if (this->deckLinkInput->EnableAudioInput(bmdAudioSampleRate48kHz, bmdAudioSampleType16bitInteger, 2) != S_OK) {
        //     std::cout << "Failed to enable audio stream." << std::endl;
        //     return;
        // }

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
    IDeckLinkVideoFrameAncillary* anc;
    void * ancData;

    if (videoFrame->GetAncillaryData(&anc) == S_OK) {
        std::cout << "Anc" << anc->GetDisplayMode() << std::endl;
        

        if (anc->GetBufferForVerticalBlankingLine(4, &ancData) == S_OK) {
            std::cout << "Line" << std::endl;
        }

        anc->Release();
    }


    std::cout << std::flush;

    return S_OK;
}

HRESULT
DeckLinkReceiver::VideoInputFormatChanged(BMDVideoInputFormatChangedEvents notificationEvents, IDeckLinkDisplayMode *newDisplayMode, BMDDetectedVideoInputFormatFlags detectedSignalFlags)
{
	auto threadId = std::this_thread::get_id();
   	std::cout << "VideoInputFormatChanged threadId: " << threadId << std::endl;

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
