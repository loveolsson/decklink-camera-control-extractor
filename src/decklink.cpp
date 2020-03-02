#include "decklink.h"
#include "include/DeckLinkAPI.h"


IDeckLink *GetFirstDeckLink() {
    IDeckLink* deckLink = nullptr;
    
    IDeckLinkIterator *deckLinkIterator = CreateDeckLinkIteratorInstances();

    if (deckLinkIterator->Next(&deckLink) != S_OK) {
        deckLink = nullptr;
    }

    deckLinkIterator->Release();

    return deckLink;
}