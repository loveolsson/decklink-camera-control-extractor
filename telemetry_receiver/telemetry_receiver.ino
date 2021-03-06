#include "receiver.h"
#include "runstate.h"
#include "sender.h"

#include "esp32_defines.h"

TaskHandle_t taskSerial;
TaskHandle_t taskTwi;

RunState runState;

void
setup()
{
    printf("Starting...\n");

    SetupPins();

    if (false) {
        SenderInit(&runState);
        xTaskCreatePinnedToCore(SenderWireLoop, "Sender wire loop", 10000, &runState, 1, &taskTwi,
                                0);
        xTaskCreatePinnedToCore(SenderSerialLoop, "Sender serial loop", 10000, &runState, 1,
                                &taskSerial, 1);
    } else {
        ReceiverInit(&runState);
        xTaskCreatePinnedToCore(ReceiverWireLoop, "Receiver wire loop", 10000, &runState, 1,
                                &taskTwi, 0);
        xTaskCreatePinnedToCore(ReceiverSerialLoop, "Receiver serial loop", 10000, &runState, 1,
                                &taskSerial, 1);
    }
}

void
loop()
{
}
