/**
 * @file uart.hpp
 * @brief Declaration of the Uart class for managing UART communication on the ESP32-C6.
 * 
 * This class wraps the ESP-IDF UART driver and provides a simplified interface for
 * asynchronous communication, including features like automatic newline expansion and
 * task-based send/receive handling integrated with message queues.
 * 
 * Usage:
 * - Call `init()` to initialize the UART driver and bind a FreeRTOS event queue.
 * - Use `send()` to transmit data. Enable newline expansion via `setNewlineExpansion(true)`.
 * - Use `receive()` or `receiveTask()` to get input from UART and push it to the parser queue.
 * 
 * @note Integrates with MessageHandler to send/receive application-level messages.
 * @note Designed to be used with tasks in a FreeRTOS environment.
 * @note Supports interactive input (e.g., echo + backspace) for terminal usability.
 * 
 * @author meths1
 * @date 21.03.2025
 */


#pragma once

#include "driver/uart.h"
#include "driver/gpio.h"

#include "MessageHandler.hpp"

namespace uartConfig {

/**
 * @brief Constants/defaults for UART communication.
 */

/* Defines the size of the UART ring buffer */
constexpr unsigned int UART_BUFFER_SIZE = 256;
/* Defines the maximum number of events that can be held in the event queue */
constexpr int EVENT_QUEUE_SIZE = 20;
constexpr uart_port_t DEFAULT_PORT = UART_NUM_0;
constexpr gpio_num_t DEFAULT_TX_PIN = GPIO_NUM_16;
constexpr gpio_num_t DEFAULT_RX_PIN = GPIO_NUM_17;
constexpr uart_config_t DEFAULT_CONFIG = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .rx_flow_ctrl_thresh = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_DEFAULT,
    .flags = {allow_pd: false, backup_before_sleep: false}
};
} // namespace uart

/**
 * @brief UART class for handling UART communication.
 */
class Uart {
public:
    /**
     * @brief Construct a new Uart object.
     * 
     * @param port UART port number. Default is DEFAULT_PORT.
     * @param config UART configuration. Default is DEFAULT_CONFIG.
     * @param tx_pin GPIO number for TX pin. Default is DEFAULT_TX_PIN.
     * @param rx_pin GPIO number for RX pin. Default is DEFAULT_RX_PIN.
     */
    Uart(uart_port_t port = uartConfig::DEFAULT_PORT, uart_config_t config = uartConfig::DEFAULT_CONFIG, 
        gpio_num_t tx_pin = uartConfig::DEFAULT_TX_PIN, gpio_num_t rx_pin = uartConfig::DEFAULT_RX_PIN);

    /**
     * @brief Destroy the Uart object.
     */
    ~Uart();

    /**
     * @brief Initializes the UART interface and installs the driver.
     *
     * @param uart_queue Pointer to a queue handle for receiving UART events.
     */
    void init(QueueHandle_t* uart_queue);

    /**
     * @brief Task function to transmit data from a queue to UART.
     *
     * This task continuously reads messages from the UART_QUEUE and writes them to UART.
     * Meant to be run inside a FreeRTOS task.
     *
     * @param msgHandler Pointer to MessageHandler used to access the queue.
     */
    void sendTask(MessageHandler* msgHandler);

    /**
     * @brief Task function to read characters from UART and buffer input.
     *
     * Reads character-by-character until a newline (`\n`) or carriage return (`\r`) is received.
     * Handles backspace correctly by editing the input buffer and updating terminal output.
     * Forwards the final message to the DATA_PARSER_QUEUE for parsing.
     *
     * @param msgHandler Pointer to MessageHandler used to enqueue parsed messages.
     */
    void receiveTask(MessageHandler* msgHandler);

    /**
     * @brief Send raw data over UART.
     *
     * If newline expansion is enabled, '\r' will be followed by '\n'.
     *
     * @param data Pointer to the data to be sent.
     * @param len Length of the data.
     * @return Number of bytes written, or -1 on error.
     */
    int send(const char *data, size_t len);

     /**
     * @brief Receive data from UART event.
     *
     * Waits for UART_RX event and returns the received bytes.
     *
     * @param data Buffer to store received data.
     * @return Number of bytes received.
     */
    size_t receive(char *data);
    
    /**
     * @brief Enable or disable newline expansion for UART output.
     *
     * When enabled, each '\r' is followed by '\n' for proper terminal formatting.
     *
     * @param enable True to enable, false to disable.
     */
    void setNewlineExpansion(bool enable)
    {
        _expandNewline = enable;
    }

private:
    uart_port_t _port;              // UART port number.
    uart_config_t _uart_config;     // UART configuration structure.
    gpio_num_t _tx_pin;             // GPIO number for TX pin.
    gpio_num_t _rx_pin;             // GPIO number for RX pin.
    QueueHandle_t* _uart_queue;     // Pointer to the UART event queue object.
    static constexpr size_t TX_BUF_SIZE = 256;  // Size of the UART TX buffer.
    char _txBuf[TX_BUF_SIZE];       // UART TX buffer.
    bool _expandNewline = true;     // Flag to enable/disable CRLF expansion.

    /**
     * @brief Internal UART event handler that reads data on RX events.
     *
     * @param data Buffer to store received characters.
     * @return Number of bytes received.
     */
    size_t uart_event_handler(char* data);

};
