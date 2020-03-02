#include "decklink.h"
#include <iostream>

IDeckLink *GetFirstDeckLink() {
    IDeckLink* deckLink = nullptr;
    
    IDeckLinkIterator *deckLinkIterator = CreateDeckLinkIteratorInstance();

    if (deckLinkIterator->Next(&deckLink) != S_OK) {
        deckLink = nullptr;
    }

    deckLinkIterator->Release();

    return deckLink;
}

DeckLinkReceiver::DeckLinkReceiver(IDeckLink* _deckLink) 
    : deckLink(_deckLink) 
{
    if (this->deckLink->QueryInterface(IID_IDeckLinkInput, (void**)&this->deckLinkInput) == S_OK) {
        std::cout << "DeckLink card has input" << std::endl;
    }

    
}

DeckLinkReceiver::~DeckLinkReceiver() {
    if (this->deckLinkInput) {
        std::cout << "Releasing input." << std::endl;
        this->deckLinkInput->Release();
    }

    if (this->deckLink) {
        std::cout << "Releasing card." << std::endl;
        this->deckLink->Release();
    }
}
