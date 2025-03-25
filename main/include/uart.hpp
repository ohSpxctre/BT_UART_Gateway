/**
 * @file uart.hpp
 * @brief Header file for UART communication class.
 * 
 * This file contains the declaration of the Uart class, which provides methods
 * for initializing, sending, and receiving data over UART.
 * 
 * The Uart class encapsulates the UART configuration, port, and GPIO pins for
 * TX and RX. It provides methods to initialize the UART with a given event queue,
 * send data, and receive data.
 * 
 * The default UART configuration, port, and GPIO pins are defined as constants.
 * 
 * @note This class is designed to work with the ESP-IDF framework.
 * 
 * @author meths1
 * @date 21.03.2025
 */

#pragma once

#include "driver/uart.h"
#include "driver/gpio.h"
#include "MessageQueue.h"

/**
 * @brief Constants/defaults for UART communication.
 */
/* Defines the size of the UART ring buffer */
static const int UART_BUFFER_SIZE = 1024;
/* Defines the maximum number of events that can be held in the event queue */
static const int EVENT_QUEUE_SIZE = 10;
static const uart_config_t DEFAULT_CONFIG = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .rx_flow_ctrl_thresh = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_DEFAULT
};
static const uart_port_t DEFAULT_PORT = UART_NUM_0;
static const gpio_num_t DEFAULT_TX_PIN = GPIO_NUM_4;
static const gpio_num_t DEFAULT_RX_PIN = GPIO_NUM_5;

/**
 * @brief UART class for handling UART communication.
 */
class Uart {
private:
    uart_port_t _port;              // UART port number.
    uart_config_t _uart_config;     // UART configuration structure.
    gpio_num_t _tx_pin;             // GPIO number for TX pin.
    gpio_num_t _rx_pin;             // GPIO number for RX pin.
    rtos::MessageQueue<uart_event_t>* _uart_queue; // Pointer to the UART event queue object.

    /**
     * @brief Handle UART events and process received data.
     * 
     * @param data Pointer to the buffer where received data will be stored.
     * @return Number of bytes received.
     */
    size_t uart_event_handler(char* data);

public:
    /**
     * @brief Construct a new Uart object.
     * 
     * @param port UART port number. Default is DEFAULT_PORT.
     * @param config UART configuration. Default is DEFAULT_CONFIG.
     * @param tx_pin GPIO number for TX pin. Default is DEFAULT_TX_PIN.
     * @param rx_pin GPIO number for RX pin. Default is DEFAULT_RX_PIN.
     */
    Uart(uart_port_t port = DEFAULT_PORT, uart_config_t config = DEFAULT_CONFIG, gpio_num_t tx_pin = DEFAULT_TX_PIN, gpio_num_t rx_pin = DEFAULT_RX_PIN);

    /**
     * @brief Destroy the Uart object.
     */
    ~Uart();

    /**
     * @brief Initialize the UART with the given queue.
     * 
     * @param uart_queue Pointer to the UART event queue handle.
     */
    void init(rtos::MessageQueue<uart_event_t>* uart_queue);

    /**
     * @brief Send data over UART.
     * 
     * @param data Pointer to the data to be sent.
     * @return Number of bytes sent.
     */
    int send(const char *data);

    /**
     * @brief Receive data from UART through the event handler.
     * 
     * @param data Pointer to the buffer where received data will be stored.
     * @return Number of bytes received.
     */
    int receive(char *data);
};

