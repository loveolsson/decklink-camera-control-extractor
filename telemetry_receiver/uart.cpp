#include "uart.h"

#include "driver/uart.h"

#include "esp32_defines.h"

static QueueHandle_t uart_queue;

uint8_t
CRC(uint8_t *data, size_t size)
{
    uint8_t sum = 0;

    for (size_t i = 0; i < size; ++i) {
        sum ^= data[i];
    }

    return sum;
}

void
InitUART()
{
    const int uart_buffer_size = (1024 * 2);

    uart_config_t uart_config = {};
    uart_config.baud_rate     = 9600;
    uart_config.data_bits     = UART_DATA_8_BITS;
    uart_config.parity        = UART_PARITY_DISABLE;
    uart_config.stop_bits     = UART_STOP_BITS_1;
    uart_config.flow_ctrl     = UART_HW_FLOWCTRL_DISABLE;

    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_config));
    ESP_ERROR_CHECK(
        uart_set_pin(UART_PORT, UART_TX, UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(
        uart_driver_install(UART_PORT, uart_buffer_size, uart_buffer_size, 10, &uart_queue, 0));
}

size_t
UARTAvailable()
{
    size_t ret;
    ESP_ERROR_CHECK(uart_get_buffered_data_len(UART_PORT, &ret));
    return ret;
}

uint8_t
UARTReadBytes(uint8_t *data, const size_t size)
{
    return uart_read_bytes(UART_PORT, data, size, 100);
}

uint8_t
UARTReadOneByte()
{
    uint8_t ret;
    if (UARTReadBytes(&ret, sizeof(ret)) == 1) {
        return ret;
    }
    return 0;
}
