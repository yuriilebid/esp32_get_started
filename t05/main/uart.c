#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "driver/uart.h"
#include "string.h"

#define TXPIN 16
#define RXPIN 17

void app_main() {
    const int uart_buffer_size = (1024 * 2);

    uart_config_t uart_cfg = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,
        .rx_flow_ctrl_thresh = 122
    };
    uart_param_config(UART_NUM_2, &uart_cfg);

    uart_set_pin(UART_NUM_2, RXPIN, TXPIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    uart_driver_install(UART_NUM_2, uart_buffer_size, 0, 0, NULL, 0);
    char* str = "\e[41mRED\e[0m \e[42mGREEN\e[0m \e[44mBLUE\e[0m \e[0mDEFAULT\r\n";
    uart_write_bytes(UART_NUM_2, str, strlen(str));
}