#include "decklink.h"

#include "include/DeckLinkAPIDispatch.cpp"


IDeckLink *GetFirstDeckLink() {
    IDeckLink* deckLink = nullptr;
    
    IDeckLinkIterator *deckLinkIterator = CreateDeckLinkIteratorInstance();

    if (deckLinkIterator->Next(&deckLink) != S_OK) {
        deckLink = nullptr;
    }

    deckLinkIterator->Release();

    return deckLink;
}