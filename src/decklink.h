#pragma once

#include "include/DeckLinkAPI.h"
#include "mutexfifo.h"
#include "defines.h"
#include "dlwrapper.h"

#include <vector>
#include <chrono>

DLWrapper<IDeckLink> GetDeckLinkByNameOrFirst(const std::string &name);

class DeckLinkReceiver : public IDeckLinkInputCallback
{
public:
    DeckLinkReceiver(DLWrapper<IDeckLink> deckLink, ByteFifo &_fifo);
    ~DeckLinkReceiver();

    HRESULT VideoInputFrameArrived(IDeckLinkVideoInputFrame *videoFrame, IDeckLinkAudioInputPacket *audioPacket);
    HRESULT VideoInputFormatChanged(BMDVideoInputFormatChangedEvents notificationEvents, IDeckLinkDisplayMode *newDisplayMode, BMDDetectedVideoInputFormatFlags detectedSignalFlags);

    virtual HRESULT QueryInterface(REFIID, void **) final;
    virtual ULONG AddRef() final;
    virtual ULONG Release() final;

private:
    DLWrapper<IDeckLinkInput, true> wDeckLinkInput;
    bool requires10bit;
    ByteFifo &fifo;
    std::vector<uint8_t> activeTallyData;
    std::chrono::steady_clock::time_point lastTallyUpdate;
};