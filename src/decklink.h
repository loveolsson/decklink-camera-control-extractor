#pragma once

#include "include/DeckLinkAPI.h"

IDeckLink *GetFirstDeckLink();

class DeckLinkReceiver : public IDeckLinkInputCallback {
public:
    DeckLinkReceiver(IDeckLink *_deckLink);
    ~DeckLinkReceiver();

    HRESULT VideoInputFrameArrived (IDeckLinkVideoInputFrame *videoFrame, IDeckLinkAudioInputPacket *audioPacket);
    HRESULT VideoInputFormatChanged (BMDVideoInputFormatChangedEvents notificationEvents, IDeckLinkDisplayMode *newDisplayMode, BMDDetectedVideoInputFormatFlags detectedSignalFlags);


    virtual HRESULT QueryInterface(REFIID, void**) = 0;

private:
    IDeckLink* deckLink;
    IDeckLinkInput* deckLinkInput = nullptr;
};