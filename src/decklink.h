#pragma once

#include "include/DeckLinkAPI.h"
#include "mutexfifo.h"

IDeckLink *GetFirstDeckLink();

class DeckLinkReceiver : public IDeckLinkInputCallback {
public:
    DeckLinkReceiver(IDeckLink *_deckLink, ByteFifo *_fifo);
    ~DeckLinkReceiver();

    HRESULT VideoInputFrameArrived (IDeckLinkVideoInputFrame *videoFrame, IDeckLinkAudioInputPacket *audioPacket);
    HRESULT VideoInputFormatChanged (BMDVideoInputFormatChangedEvents notificationEvents, IDeckLinkDisplayMode *newDisplayMode, BMDDetectedVideoInputFormatFlags detectedSignalFlags);

    virtual HRESULT QueryInterface(REFIID, void**) final;
    virtual ULONG AddRef() final;
    virtual ULONG Release() final;

private:
    IDeckLink* deckLink;
    IDeckLinkInput* deckLinkInput = nullptr;
    bool requires10bit;
    ByteFifo *fifo;
};