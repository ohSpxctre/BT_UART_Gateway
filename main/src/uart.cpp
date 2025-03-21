/**
 * @file uart.cpp
 * @brief Implementation file for UART communication class.
 * 
 * This file contains the implementation of the Uart class, which provides methods
 * for initializing, sending, and receiving data over UART.
 * 
 * @note This class is designed to work with the ESP-IDF framework.
 * 
 * @author meths1
 * @date [Date]
 */

#include "uart.hpp"

#include "string.h"
#include "esp_log.h"

static const char* TAG = "UART";

/**
 * @brief Construct a new Uart object.
 * 
 * @param port UART port number.
 * @param config UART configuration.
 * @param tx_pin GPIO number for TX pin.
 * @param rx_pin GPIO number for RX pin.
 */
Uart::Uart(uart_port_t port, uart_config_t config, gpio_num_t tx_pin, gpio_num_t rx_pin) :  _port(port), _uart_config(config), _tx_pin(tx_pin), _rx_pin(rx_pin)
{
}

/**
 * @brief Destroy the Uart object.
 */
Uart::~Uart()
{
}

/**
 * @brief Initialize the UART with the given queue.
 * 
 * @param uart_queue Pointer to the UART event queue handle.
 */
void Uart::init(rtos::MessageQueue<uart_event_t>* uart_queue)
{
    _uart_queue = uart_queue;
    ESP_ERROR_CHECK(uart_param_config(_port, &_uart_config)); 
    ESP_ERROR_CHECK(uart_set_pin(_port, (int) _tx_pin, (int) _rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    QueueHandle_t queueHandle = _uart_queue->handle();
    ESP_ERROR_CHECK(uart_driver_install(_port, UART_BUFFER_SIZE, UART_BUFFER_SIZE, EVENT_QUEUE_SIZE, &queueHandle, 0)); 
}

/**
 * @brief Send data over UART.
 * 
 * @param data Pointer to the data to be sent.
 * @return Number of bytes sent.
 */
int Uart::send(const char* data)
{
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(_port, data, len);
    if (txBytes < 0) {
        ESP_LOGI(TAG, "Failed to send data: %s", data);
    }
    return txBytes;
}

/**
 * @brief Receive data from UART.
 * 
 * @param data Pointer to the buffer where received data will be stored.
 * @return Number of bytes received.
 */
int Uart::receive(char* data)
{
    return uart_event_handler(data);
}

/**
 * @brief Handle UART events and process received data.
 * 
 * @param data Pointer to the buffer where received data will be stored.
 * @return Number of bytes received.
 */
size_t Uart::uart_event_handler(char* data)
{
    uart_event_t event;
    size_t receivedBytes = 0;
    // Waiting for UART event.
    if (_uart_queue->receive(event))
    {
        switch(event.type) {
            // Event of UART receiving data
            case UART_DATA:
                receivedBytes = event.size;
                ESP_LOGI(TAG, "Port: %d, data length: %d", _port, receivedBytes);
                uart_read_bytes(_port, &data, receivedBytes, portMAX_DELAY);
                break;
            // Event of HW FIFO overflow detected
            case UART_FIFO_OVF:
                ESP_LOGI(TAG, "Port: %d, FIFO overflow", _port);
                uart_flush_input(_port);
                xQueueReset(_uart_queue->handle());
                break;
            // Event of UART ring buffer full
            case UART_BUFFER_FULL:
                ESP_LOGI(TAG, "Port: %d, Ring buffer full", _port);
                uart_flush_input(_port);
                xQueueReset(_uart_queue->handle());
                break;
            // Event of UART RX break detected
            case UART_BREAK:
                ESP_LOGI(TAG, "Port: %d, rx break", _port);
                break;
            // Event of UART parity check error
            case UART_PARITY_ERR:
                ESP_LOGI(TAG, "Port: %d, parity error", _port);
                break;
            // Event of UART frame error
            case UART_FRAME_ERR:
                ESP_LOGI(TAG, "Port: %d, frame error", _port);
                break;
            // UART_PATTERN_DET
            case UART_PATTERN_DET:
                ESP_LOGI(TAG, "Port: %d, pattern detected", _port);
                break;
            // Others
            default:
                ESP_LOGI(TAG, "Port: %d, event type: %d", _port, event.type);
                break;
        }
    }
    return receivedBytes;
}