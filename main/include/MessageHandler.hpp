/**
 * @file MessageHandler.hpp
 * @brief Declaration of the MessageHandler class for managing message queues between components.
 * 
 * The MessageHandler class encapsulates multiple FreeRTOS queues used for communication
 * between components like UART, BLE, and data parsing. It allows categorized message routing
 * and supports metadata (ParserMessageID) for identifying message sources.
 * 
 * Usage:
 * - Use `send()` to place messages into categorized queues.
 * - Use `receive()` to extract messages from them.
 * - Set the externally created UART event queue using `setUartEventQueue()`.
 * 
 * @note Only the DATA_PARSER_QUEUE supports and uses the optional `ParserMessageID`.
 * @note class is designed to be thread-safe and RTOS-compatible.
 * 
 * @author meths1
 * @date 29.03.2025
 */

#pragma once

#include <memory>
#include "rtos.h"

/**
 * @class MessageHandler
 * @brief A class to handle message queues for different communication types.
 */
class MessageHandler {
public:
    /* Default size of the message queue. */
    static constexpr size_t DefaultQueueSize = 10;

    /* Maximum length of a message. */
    static constexpr size_t MaxMessageLength = 256;

    /**
     * @enum QueueType
     * @brief Enum representing the types of queues managed by the MessageHandler.
     */
    enum class QueueType {
        UART_QUEUE,         /**< Queue for UART communication. */
        BLE_QUEUE,          /**< Queue for BLE communication. */
        DATA_PARSER_QUEUE   /**< Queue for data parser communication. */
    };

    /**
     * @typedef Message
     * @brief Alias for a fixed-size array representing a message.
     */
    using Message = std::array<char, MaxMessageLength>;

    /**
     * @enum ParserMessageID
     * @brief Enum representing the message IDs for the data parser.
     */
    enum class ParserMessageID {
        MSG_ID_UART,
        MSG_ID_BLE,
        OTHER
    };

    /**
     * @brief Constructor for the MessageHandler class.
     * @param queueSize The size of the message queues. Defaults to DefaultQueueSize.
     */
    MessageHandler(size_t queueSize = DefaultQueueSize);

    /**
     * @brief Sends a message to the specified internal message queue.
     * 
     * If the target queue is DATA_PARSER_QUEUE, the optional `id` parameter will be attached to the message.
     * For all other queues, `id` is ignored.
     * 
     * @param queueType The type of queue to send to.
     * @param message The message content.
     * @param id Optional: only used when queueType is DATA_PARSER_QUEUE.
     * @return true if the message was sent successfully, false otherwise.
     */
    bool send(QueueType queueType, const Message& message, ParserMessageID id = ParserMessageID::OTHER);

    /**
     * @brief Receives a message from the specified internal queue.
     * 
     * If the message comes from DATA_PARSER_QUEUE and `id` is not null, the message ID will be stored in `id`.
     * For all other queues, `id` is ignored.
     * 
     * @param queueType The type of queue to receive from.
     * @param message Output parameter to receive the message.
     * @param id Optional output parameter to retrieve the message ID for DATA_PARSER_QUEUE.
     * @return true if a message was received successfully, false otherwise.
     */
    bool receive(QueueType queueType, Message& message, ParserMessageID* id = nullptr);

    /**
     * @brief Sets an externally created UART event queue.
     * @param queue Pointer to the externally created UART event queue.
     *              The MessageHandler does not take ownership of this queue.
     */
    void setUartEventQueue(QueueHandle_t queue) {
        _uartEventQueue = queue;
    }

    /**
     * @brief Gets the UART event queue.
     * @return Pointer to the UART event queue.
     */
    QueueHandle_t* getUartEventQueue() {
        return &_uartEventQueue;
    }

private:
    /**
     * @struct ParserMessage
     * @brief Struct representing a message with an associated ID for the data parser.
     */
    struct ParserMessage {
        ParserMessageID id;
        Message message;
    };

    /* Shared unique to message queues for different communication types. */
    std::unique_ptr<rtos::MessageQueue<Message>> _uartQueue;
    std::unique_ptr<rtos::MessageQueue<Message>> _bleQueue;
    std::unique_ptr<rtos::MessageQueue<ParserMessage>> _dataParserQueue;

    /* Pointer to the externally created UART event queue. */
    QueueHandle_t _uartEventQueue = nullptr;  // Not owned

    /**
     * @brief Internal helper to retrieve the appropriate message queue.
     * 
     * Only supports UART_QUEUE and BLE_QUEUE. For DATA_PARSER_QUEUE,
     * access is handled separately and not through this method.
     * 
     * @param queueType The queue type to retrieve.
     * @return Reference to the matching message queue.
     */
    std::unique_ptr<rtos::MessageQueue<Message>>& getQueue(QueueType queueType);
};
