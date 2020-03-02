#pragma once

#include "include/DeckLinkAPI.h"

IDeckLink *GetFirstDeckLink();

class DeckLinkReceiver {
public:
    DeckLinkReceiver(IDeckLink *_deckLink);
    ~DeckLinkReceiver();

private:
    IDeckLink* deckLink;
    IDeckLinkInput* deckLinkInput;
};