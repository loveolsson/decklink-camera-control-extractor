#pragma once

#include "include/DeckLinkAPI.h"
#include "mutexfifo.h"
#include "defines.h"

#include <vector>
#include <chrono>

IDeckLink *GetDeckLinkByNameOrFirst(std::string &name);

class DeckLinkReceiver : public IDeckLinkInputCallback
{
public:
    DeckLinkReceiver(IDeckLink *deckLink, ByteFifo *_fifo);
    ~DeckLinkReceiver();

    HRESULT VideoInputFrameArrived(IDeckLinkVideoInputFrame *videoFrame, IDeckLinkAudioInputPacket *audioPacket);
    HRESULT VideoInputFormatChanged(BMDVideoInputFormatChangedEvents notificationEvents, IDeckLinkDisplayMode *newDisplayMode, BMDDetectedVideoInputFormatFlags detectedSignalFlags);

    virtual HRESULT QueryInterface(REFIID, void **) final;
    virtual ULONG AddRef() final;
    virtual ULONG Release() final;

private:
    Wrapper<IDeckLinkInput, true> wDeckLinkInput;
    bool requires10bit;
    ByteFifo *fifo;
    std::vector<uint8_t> activeTallyData;
    std::chrono::steady_clock::time_point lastTallyUpdate;
};