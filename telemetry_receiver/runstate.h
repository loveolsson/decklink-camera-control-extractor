#pragma once

#include "include/mutexfifo.h"

#include "esp32_defines.h"

#include <memory>

namespace BMD {
class SDICameraControl;
class SDITallyControl;
}  // namespace BMD

template <typename T>
class I2CCustom;

struct RunState {
    std::shared_ptr<I2CCustom<BMD::SDICameraControl>> cc;
    std::shared_ptr<I2CCustom<BMD::SDITallyControl>> tally;
    ByteFifo queue;
};
