#pragma once
#include "driver/gpio.h"
#include "include/defines.h"

#ifdef ARDUINO_ARCH_ESP32  // A safeguard against compiling for the wrong architecture
#define DIP_PIN_1 GPIO_NUM_2
#define DIP_PIN_2 GPIO_NUM_4
#define DIP_PIN_3 GPIO_NUM_5
#define DIP_PIN_4 GPIO_NUM_18
#define RS422_DE GPIO_NUM_12
#define RS422_RE GPIO_NUM_14
#define TWI_CL GPIO_NUM_22
#define TWI_DA GPIO_NUM_21
#define TWI_EN GPIO_NUM_23
#define LED_ERR GPIO_NUM_19
#define OPTO_O1 GPIO_NUM_25
#define OPTO_O2 GPIO_NUM_33
#define UART_TX GPIO_NUM_17
#define UART_RX GPIO_NUM_16
#define UART_PORT UART_NUM_2
#define SHIELD_ADDR 0x6E
#endif

void SetupPins();
uint8_t ReadDips();
