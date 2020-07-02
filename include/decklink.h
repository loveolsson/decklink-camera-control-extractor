#pragma once
#include "include/DeckLinkAPI.h"
#include "mutexfifo.h"

#include <stdint.h>

#include <chrono>
#include <iosfwd>
#include <memory>
#include <vector>

template <typename T>
class DLWrapper;

std::shared_ptr<DLWrapper<IDeckLinkInput>> GetDeckLinkByNameOrFirst(const std::string &name);

class DeckLinkReceiver : public IDeckLinkInputCallback
{
public:
    DeckLinkReceiver(std::shared_ptr<DLWrapper<IDeckLinkInput>> _wDeckLinkInput, ByteFifo &_fifo);
    ~DeckLinkReceiver();

    HRESULT VideoInputFrameArrived(IDeckLinkVideoInputFrame *videoFrame,
                                   IDeckLinkAudioInputPacket *audioPacket);
    HRESULT VideoInputFormatChanged(BMDVideoInputFormatChangedEvents notificationEvents,
                                    IDeckLinkDisplayMode *newDisplayMode,
                                    BMDDetectedVideoInputFormatFlags detectedSignalFlags);

    virtual HRESULT QueryInterface(REFIID, void **) final;
    virtual ULONG AddRef() final;
    virtual ULONG Release() final;

private:
    std::shared_ptr<DLWrapper<IDeckLinkInput>> wDeckLinkInput;
    bool requires10bit;
    ByteFifo &fifo;
    std::vector<uint8_t> activeTallyData;
    std::chrono::steady_clock::time_point lastTallyUpdate;
};