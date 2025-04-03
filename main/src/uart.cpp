/**
 * @file uart.cpp
 * @brief Implementation of the Uart class for UART communication on ESP32-C6.
 * 
 * This file defines the logic for the UART driver wrapper used in the application,
 * including initialization, sending, and receiving data via FreeRTOS queues.
 * It supports backspace handling, newline expansion, and buffering of UART data
 * for further parsing or forwarding.
 * 
 * @note This implementation is designed for use with the ESP-IDF framework.
 * @note The class supports optional expansion of `\r`/`\n` to `\r\n` sequences to ensure terminal compatibility.
 * @note The receive task echoes characters and supports backspace processing for interactive input.
 * 
 * @author meths1
 * @date 21.03.2025
 */


#include "uart.hpp"

#include "string.h"
#include "esp_log.h"

constexpr const char* TAG = "UART";

namespace uart {

Uart::Uart(uart_port_t port, uart_config_t config, gpio_num_t tx_pin, gpio_num_t rx_pin) :  _port(port), _uart_config(config), _tx_pin(tx_pin), _rx_pin(rx_pin)
{
}

Uart::~Uart()
{
    // Delete the UART driver
    ESP_ERROR_CHECK(uart_driver_delete(_port));
    ESP_LOGI(TAG, "UART driver deleted for port %d", _port);

    // Set the UART queue pointer to null
    _uart_queue = nullptr;
    ESP_LOGI(TAG, "UART queue pointer set to null for port %d", _port);

    // Clear the TX buffer
    _txBuf[0] = '\0';
    ESP_LOGI(TAG, "UART TX buffer cleared for port %d", _port);
}

void Uart::init(QueueHandle_t* uart_queue)
{
    _uart_queue = uart_queue;
    ESP_ERROR_CHECK(uart_param_config(_port, &_uart_config)); 
    ESP_ERROR_CHECK(uart_set_pin(_port, (int) _tx_pin, (int) _rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(_port, 2 * UART_BUFFER_SIZE, 2 * UART_BUFFER_SIZE, EVENT_QUEUE_SIZE, _uart_queue, 0)); 
}

void Uart::sendTask(MessageHandler* msgHandler)
{
    // Get message from the UART queue and send it to the UART interface
    MessageHandler::Message message;

    if (msgHandler->receive(MessageHandler::QueueType::UART_QUEUE, message)) {
        this->send(message.data(), message.size());
        ESP_LOGI(TAG, "Sent data: %s", message.data());
    } else {
        ESP_LOGW(TAG, "Failed to receive message from UART_QUEUE");
    }
}

/**
 * @brief Reads user input from UART, handling special characters like backspace,
 *        and sends complete messages to the data parser.
 * 
 * This function echoes characters as they arrive, supports editing via backspace,
 * and detects end-of-line sequences (`\r` or `\n`) to terminate the input.
 */

void Uart::receiveTask(MessageHandler* msgHandler)
{
    char data[uart::UART_BUFFER_SIZE];
    MessageHandler::Message message;
    
    int bytesReceived = 0;
    bool isEndOfLine = false;

    // Clear the message buffer
    std::fill(data, data + uart::UART_BUFFER_SIZE, '\0');
    std::fill(message.begin(), message.end(), '\0');

    while (!isEndOfLine && bytesReceived < uart::UART_BUFFER_SIZE - 1) {
        int received = this->receive(data + bytesReceived);
        for (int i = 0; i < received; ++i) {
            char ch = data[bytesReceived + i];

            // Handle backspace or delete key
            if (ch == '\b' || ch == 0x7F) {
                if (bytesReceived > 0) {
                    bytesReceived--;
                    this->send("\b \b", 3);
                }
                continue;
            }

            // Echo back each character (local terminal-style interaction)
            this->send(&ch, 1);

            // Store character
            data[bytesReceived++] = ch;

            // Check for newline or carriage return
            if (ch == '\r' || ch == '\n') {
                isEndOfLine = true;
                break; // Skip rest of received buffer
            }
        }
    }

    std::copy(data, data + bytesReceived, message.begin());
    msgHandler->send(MessageHandler::QueueType::DATA_PARSER_QUEUE, message, MessageHandler::ParserMessageID::MSG_ID_UART);

    ESP_LOGI(pcTaskGetName(nullptr), "Received data: %s", data);
    
}

int Uart::send(const char* data, size_t len)
{
    size_t txPos = 0;
    int totalWritten = 0;

    if (_expandNewline) {
        for (size_t i = 0; i < len; ++i) {
            char ch = data[i];

            if (ch == '\r') {
                _txBuf[txPos++] = '\r';
                // Check if next char is '\n' — keep it as-is
                if ((i + 1 < len) && data[i + 1] == '\n') {
                    _txBuf[txPos++] = '\n';
                    ++i; // Skip the '\n'
                } else {
                    _txBuf[txPos++] = '\n';
                }
            }
            else if (ch == '\n') {
                // Lone \n, convert to \r\n
                _txBuf[txPos++] = '\r';
                _txBuf[txPos++] = '\n';
            }
            else {
                _txBuf[txPos++] = ch;
            }

            if (txPos >= TX_BUF_SIZE - 2) {
                int written = uart_write_bytes(_port, _txBuf, txPos);
                if (written < 0) return -1;
                totalWritten += written;
                txPos = 0;
            }
        }

        if (txPos > 0) {
            int written = uart_write_bytes(_port, _txBuf, txPos);
            if (written < 0) return -1;
            totalWritten += written;
        }
    } else {
        totalWritten = uart_write_bytes(_port, data, len);
    }

    return totalWritten;
}

size_t Uart::receive(char* data)
{
    return uart_event_handler(data);
}

size_t Uart::uart_event_handler(char* data)
{
    uart_event_t event;

    // Waiting for UART event.
    if (xQueueReceive(*_uart_queue, (void *)&event, portMAX_DELAY))
    {
        bzero(data, (size_t)sizeof(data));
        switch(event.type) {
            // Event of UART receiving data
            case UART_DATA:
                ESP_LOGI(TAG, "Port: %d, data length: %d", _port, event.size);
                uart_read_bytes(_port, data, event.size, portMAX_DELAY);
                break;
            // Event of HW FIFO overflow detected
            case UART_FIFO_OVF:
                ESP_LOGI(TAG, "Port: %d, FIFO overflow", _port);
                uart_flush_input(_port);
                xQueueReset(*_uart_queue);
                break;
            // Event of UART ring buffer full
            case UART_BUFFER_FULL:
                ESP_LOGI(TAG, "Port: %d, Ring buffer full", _port);
                uart_flush_input(_port);
                xQueueReset(*_uart_queue);
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
    return event.size;
}

void Uart::setNewlineExpansion(bool enable)
{
    _expandNewline = enable;
}

} // namespace uart