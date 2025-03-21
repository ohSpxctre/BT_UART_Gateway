#pragma once

#include "driver/uart.h"


static const int RX_BUF_SIZE = 1024;

class Uart
{
private:
    void uart_config_t uart_config;
    {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    }
    
public:
    Uart(/* args */);
    ~Uart();

    void init();
    void send(const char *data);
    void receive(char *data);
};

